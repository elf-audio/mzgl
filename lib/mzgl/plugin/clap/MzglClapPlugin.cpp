// CLAP wrapper implementation: clap_entry + factory + one clap_plugin wrapping
// the mzgl ::Plugin from instantiatePlugin(), with audio-ports, note-ports
// (raw MIDI dialect, so CC flows straight to midiReceivedAtTime - no proxy
// parameter layer like VST3's MzglVST3MidiCC), params (CLAP params are plain
// value ranges, no normalisation dance), streamed state, and on Linux a gui
// over plugin/linux/X11PluginView pumped by the host's timer + posix-fd
// extensions. Mirrors MzglVST3SingleComponent / KoalaVST3Plugin behaviour.
#include "MzglClapPlugin.h"

#include "Plugin.h"
#include "PluginEditor.h"
#include "PluginParameter.h"
#include "MidiMessage.h"
#include "log.h"
#include "filesystem.h"
#include "util.h"

#if defined(__linux__) && !defined(__ANDROID__)
#	include "X11PluginView.h"
#	include <dlfcn.h>
#endif

#include <clap/clap.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <mutex>
#include <string>

namespace mzglclap {

	namespace {

		void copyString(char *dst, size_t dstSize, const char *src) {
			snprintf(dst, dstSize, "%s", src);
		}

		// A host-visible parameter change made from the editor, queued on whatever
		// thread the UI runs on and drained into the host's out-events on the
		// audio thread (process) or main thread (params.flush).
		struct PendingParamEvent {
			enum class Kind { Value, GestureBegin, GestureEnd };
			Kind kind;
			clap_id paramId;
			double value;
		};

	} // namespace

	struct MzglClapPlugin {
		clap_plugin_t clapPlugin {};
		const clap_host_t *host {nullptr};
		const ClapSpec *spec {nullptr};

		std::shared_ptr<::Plugin> plugin;
		int numOutputPorts {1};

		FloatBuffer interleavedIn;
		std::vector<FloatBuffer> interleavedOuts;

		std::mutex pendingMutex;
		std::vector<PendingParamEvent> pending;

#if defined(__linux__) && !defined(__ANDROID__)
		std::unique_ptr<mzgl::X11PluginView> view;
	mzglvst::ViewConfig viewCfg;
		float guiScale {0.f}; // 0 = host never told us, detect
		uint32_t guiWidth {0};
		uint32_t guiHeight {0};
		clap_id frameTimerId {CLAP_INVALID_ID};
		int registeredFd {-1};
#endif

		// ----- lifecycle -------------------------------------------------------

		bool init() {
			plugin = instantiatePlugin();
			if (!plugin) return false;
			numOutputPorts = std::max(1, plugin->getNumOutputBusses());

			plugin->sendUpdatedParameterToHost = [this](unsigned int i, float value) {
				if (!plugin || i >= plugin->getNumParams()) return;
				const bool wrap = !plugin->isInHostGesture(i);
				{
					std::lock_guard<std::mutex> lock(pendingMutex);
					if (wrap) pending.push_back({PendingParamEvent::Kind::GestureBegin, i, 0});
					pending.push_back({PendingParamEvent::Kind::Value, i, value});
					if (wrap) pending.push_back({PendingParamEvent::Kind::GestureEnd, i, 0});
				}
				requestFlush();
			};
			plugin->sendParameterGestureToHost = [this](unsigned int i, bool began) {
				if (!plugin || i >= plugin->getNumParams()) return;
				{
					std::lock_guard<std::mutex> lock(pendingMutex);
					pending.push_back(
						{began ? PendingParamEvent::Kind::GestureBegin : PendingParamEvent::Kind::GestureEnd,
						 i,
						 0});
				}
				requestFlush();
			};
		// Not processing yet: products that gate async preset/scene handoff on
		// isRunning (KoalaFX) apply state directly until activate() flips it.
		if (spec->onActivate) spec->onActivate(plugin, false);

			interleavedIn.reserve(8192 * 2);
			return true;
		}

		void destroy() {
			guiDestroy();
			if (plugin) {
				plugin->sendUpdatedParameterToHost = nullptr;
				plugin->sendParameterGestureToHost = nullptr;
			}
			plugin.reset();
		}

		void requestFlush() {
			if (host == nullptr) return;
			auto *params = (const clap_host_params_t *) host->get_extension(host, CLAP_EXT_PARAMS);
			if (params != nullptr && params->request_flush != nullptr) params->request_flush(host);
		}

		// ----- events ----------------------------------------------------------

		void handleEvent(const clap_event_header_t *ev) {
			if (ev == nullptr || ev->space_id != CLAP_CORE_EVENT_SPACE_ID || !plugin) return;
			switch (ev->type) {
				case CLAP_EVENT_MIDI: {
					if (!spec->midiInput) break;
					auto *midi = (const clap_event_midi_t *) ev;
					plugin->midiReceivedAtTime(MidiMessage(midi->data[0], midi->data[1], midi->data[2]), ev->time);
					break;
				}
				case CLAP_EVENT_NOTE_ON: {
					if (!spec->midiInput) break;
					auto *note	 = (const clap_event_note_t *) ev;
					int velocity = std::clamp((int) std::round(note->velocity * 127.0), 1, 127);
					plugin->midiReceivedAtTime(
						MidiMessage::noteOn(std::max<int16_t>(0, note->channel) + 1, note->key, velocity),
						ev->time);
					break;
				}
				case CLAP_EVENT_NOTE_OFF: {
					if (!spec->midiInput) break;
					auto *note = (const clap_event_note_t *) ev;
					plugin->midiReceivedAtTime(
						MidiMessage::noteOff(std::max<int16_t>(0, note->channel) + 1, note->key), ev->time);
					break;
				}
				case CLAP_EVENT_PARAM_VALUE: {
					auto *pv = (const clap_event_param_value_t *) ev;
					if (pv->param_id < plugin->getNumParams()) {
						plugin->hostUpdatedParameter(pv->param_id, (float) pv->value);
					}
					break;
				}
				default: break;
			}
		}

		void applyTransport(const clap_event_transport_t *t) {
			if (t == nullptr || !plugin || !spec->needsTransport) return;
			plugin->setHostIsPlaying((t->flags & CLAP_TRANSPORT_IS_PLAYING) != 0);
			if (t->flags & CLAP_TRANSPORT_HAS_TEMPO) plugin->bpm = t->tempo;
			if (t->flags & CLAP_TRANSPORT_HAS_BEATS_TIMELINE) {
				plugin->beatPosition = (double) t->song_pos_beats / (double) CLAP_BEATTIME_FACTOR;
			}
		}

		void drainPending(const clap_output_events_t *out) {
			if (out == nullptr) return;
			std::vector<PendingParamEvent> drained;
			{
				std::lock_guard<std::mutex> lock(pendingMutex);
				drained.swap(pending);
			}
			for (const auto &p: drained) {
				if (p.kind == PendingParamEvent::Kind::Value) {
					clap_event_param_value_t ev {};
					ev.header.size	   = sizeof(ev);
					ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
					ev.header.type	   = CLAP_EVENT_PARAM_VALUE;
					ev.param_id		   = p.paramId;
					ev.cookie		   = nullptr;
					ev.note_id		   = -1;
					ev.port_index	   = -1;
					ev.channel		   = -1;
					ev.key			   = -1;
					ev.value		   = p.value;
					out->try_push(out, &ev.header);
				} else {
					clap_event_param_gesture_t ev {};
					ev.header.size	   = sizeof(ev);
					ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
					ev.header.type	   = p.kind == PendingParamEvent::Kind::GestureBegin
											 ? CLAP_EVENT_PARAM_GESTURE_BEGIN
											 : CLAP_EVENT_PARAM_GESTURE_END;
					ev.param_id		   = p.paramId;
					out->try_push(out, &ev.header);
				}
			}
		}

		// ----- process ---------------------------------------------------------

		clap_process_status process(const clap_process_t *p) {
			if (p->in_events != nullptr) {
				const uint32_t n = p->in_events->size(p->in_events);
				for (uint32_t i = 0; i < n; ++i) {
					handleEvent(p->in_events->get(p->in_events, i));
				}
			}
			applyTransport(p->transport);
			drainPending(p->out_events);

			if (!plugin || p->frames_count == 0) return CLAP_PROCESS_CONTINUE;

			const uint32_t numSamples	 = p->frames_count;
			const int channelsPerBus	 = 2;
			const size_t interleavedSize = (size_t) numSamples * channelsPerBus;

			interleavedIn.resize(interleavedSize);
			if (spec->audioInput && p->audio_inputs_count >= 1 && p->audio_inputs[0].channel_count >= 2
				&& p->audio_inputs[0].data32 != nullptr) {
				const float *left  = p->audio_inputs[0].data32[0];
				const float *right = p->audio_inputs[0].data32[1];
				for (uint32_t i = 0; i < numSamples; ++i) {
					interleavedIn[i * 2]	 = left[i];
					interleavedIn[i * 2 + 1] = right[i];
				}
			} else {
				interleavedIn.zeros();
			}

			if ((int) interleavedOuts.size() != numOutputPorts) interleavedOuts.resize(numOutputPorts);
			for (auto &o: interleavedOuts) {
				o.resize(interleavedSize);
			}

			plugin->process(&interleavedIn, interleavedOuts.data(), channelsPerBus);

			const uint32_t numOut = std::min<uint32_t>(p->audio_outputs_count, (uint32_t) numOutputPorts);
			for (uint32_t port = 0; port < numOut; ++port) {
				const clap_audio_buffer_t &out = p->audio_outputs[port];
				if (out.channel_count < 2 || out.data32 == nullptr) continue;
				float *left	 = out.data32[0];
				float *right = out.data32[1];
				if (left == nullptr || right == nullptr) continue;
				const FloatBuffer &src = interleavedOuts[port];
				for (uint32_t i = 0; i < numSamples; ++i) {
					left[i]	 = src[i * 2];
					right[i] = src[i * 2 + 1];
				}
			}
			return CLAP_PROCESS_CONTINUE;
		}

		// ----- gui (Linux/X11) -------------------------------------------------

#if defined(__linux__) && !defined(__ANDROID__)
		// The .clap is a bare ELF; its data lives in a sibling folder
		// <Name>.clap.resources/data (see mzgl_add_clap_plugin).
		static std::string bundleDataDir() {
			Dl_info info;
			if (dladdr((void *) &bundleDataDir, &info) && info.dli_fname) {
				return (fs::path(std::string(info.dli_fname) + ".resources") / "data").string();
			}
			return {};
		}

		float effectiveScale() { return guiScale > 0.f ? guiScale : mzgl::X11PluginView::detectPixelScale(); }

		bool guiCreate() {
			// Rendering and input are pumped entirely by the host: no timer
			// support, no editor (the DSP keeps running without one).
			auto *timer = (const clap_host_timer_support_t *) host->get_extension(host, CLAP_EXT_TIMER_SUPPORT);
			if (timer == nullptr || timer->register_timer == nullptr) {
				Log::e() << "MzglClapPlugin: host has no timer support - no editor";
				return false;
			}
			view			  = std::make_unique<mzgl::X11PluginView>();
		viewCfg = spec->makeViewConfig ? spec->makeViewConfig() : spec->view;
			const float scale = effectiveScale();
			guiWidth		  = (uint32_t) std::lround(viewCfg.defaultWidth * scale);
			guiHeight		  = (uint32_t) std::lround(viewCfg.defaultHeight * scale);
			return true;
		}

		void guiDestroy() {
#	if defined(__linux__) && !defined(__ANDROID__)
			if (host != nullptr) {
				if (frameTimerId != CLAP_INVALID_ID) {
					auto *timer =
						(const clap_host_timer_support_t *) host->get_extension(host, CLAP_EXT_TIMER_SUPPORT);
					if (timer != nullptr) timer->unregister_timer(host, frameTimerId);
					frameTimerId = CLAP_INVALID_ID;
				}
				if (registeredFd >= 0) {
					auto *fds = (const clap_host_posix_fd_support_t *) host->get_extension(
						host, CLAP_EXT_POSIX_FD_SUPPORT);
					if (fds != nullptr) fds->unregister_fd(host, registeredFd);
					registeredFd = -1;
				}
			}
			if (view) view->detach();
			view.reset();
#	endif
		}

		bool guiSetParent(const clap_window_t *window) {
			if (!view || window == nullptr) return false;
			mzgl::X11PluginView::Setup setup;
			setup.widthPx	 = (int) guiWidth;
			setup.heightPx	 = (int) guiHeight;
			setup.pixelScale = effectiveScale();
			setup.dataDir	 = bundleDataDir();
			setup.getPlugin	 = [this]() { return plugin; };
			if (!view->attach((uint64_t) window->x11, setup)) return false;

			auto *timer = (const clap_host_timer_support_t *) host->get_extension(host, CLAP_EXT_TIMER_SUPPORT);
			if (timer != nullptr && frameTimerId == CLAP_INVALID_ID) {
				timer->register_timer(host, 16, &frameTimerId);
			}
			auto *fds =
				(const clap_host_posix_fd_support_t *) host->get_extension(host, CLAP_EXT_POSIX_FD_SUPPORT);
			if (fds != nullptr && registeredFd < 0) {
				const int fd = view->connectionFd();
				if (fd >= 0 && fds->register_fd(host, fd, CLAP_POSIX_FD_READ)) {
					registeredFd = fd;
				}
			}
			return true;
		}
#endif
	};

	// ---------------------------------------------------------------------------
	// clap_plugin_t trampolines
	// ---------------------------------------------------------------------------

	namespace {

		MzglClapPlugin *self(const clap_plugin_t *p) {
			return (MzglClapPlugin *) p->plugin_data;
		}

		// ----- audio ports -----

		uint32_t audioPortsCount(const clap_plugin_t *p, bool isInput) {
			return isInput ? (self(p)->spec->audioInput ? 1u : 0u) : (uint32_t) self(p)->numOutputPorts;
		}

		bool audioPortsGet(const clap_plugin_t *p, uint32_t index, bool isInput, clap_audio_port_info_t *info) {
			auto *me = self(p);
			if (isInput) {
				if (!me->spec->audioInput || index != 0) return false;
				info->id = 0;
				copyString(info->name, sizeof(info->name), "Stereo In");
				info->flags = CLAP_AUDIO_PORT_IS_MAIN;
			} else {
				if (index >= (uint32_t) me->numOutputPorts) return false;
				info->id = index;
				if (me->numOutputPorts == 1) {
					copyString(info->name, sizeof(info->name), "Stereo Out");
				} else {
					copyString(info->name, sizeof(info->name), ("Output " + std::to_string(index + 1)).c_str());
				}
				info->flags = index == 0 ? CLAP_AUDIO_PORT_IS_MAIN : 0;
			}
			info->channel_count = 2;
			info->port_type		= CLAP_PORT_STEREO;
			info->in_place_pair = CLAP_INVALID_ID;
			return true;
		}

		const clap_plugin_audio_ports_t audioPortsExt = {audioPortsCount, audioPortsGet};

		// ----- note ports -----

		uint32_t notePortsCount(const clap_plugin_t *p, bool isInput) {
			return (isInput && self(p)->spec->midiInput) ? 1u : 0u;
		}

		bool notePortsGet(const clap_plugin_t *p, uint32_t index, bool isInput, clap_note_port_info_t *info) {
			if (!isInput || index != 0 || !self(p)->spec->midiInput) return false;
			info->id = 0;
			// Raw MIDI preferred: CC, pitchbend etc. reach midiReceivedAtTime
			// just like the standalone's MIDI input. CLAP notes accepted too.
			info->supported_dialects = CLAP_NOTE_DIALECT_MIDI | CLAP_NOTE_DIALECT_CLAP;
			info->preferred_dialect	 = CLAP_NOTE_DIALECT_MIDI;
			copyString(info->name, sizeof(info->name), "MIDI In");
			return true;
		}

		const clap_plugin_note_ports_t notePortsExt = {notePortsCount, notePortsGet};

		// ----- params -----

		uint32_t paramsCount(const clap_plugin_t *p) {
			auto *me = self(p);
			return me->plugin ? (uint32_t) me->plugin->getNumParams() : 0;
		}

		bool paramsGetInfo(const clap_plugin_t *p, uint32_t index, clap_param_info_t *info) {
			auto *me = self(p);
			if (!me->plugin || index >= me->plugin->getNumParams()) return false;
			auto param	= me->plugin->getParam(index);
			info->id	= index;
			info->flags = CLAP_PARAM_IS_AUTOMATABLE;
			if (param->type == PluginParameter::Type::Int || param->type == PluginParameter::Type::Indexed) {
				info->flags |= CLAP_PARAM_IS_STEPPED;
			}
			info->cookie = nullptr;
			copyString(info->name, sizeof(info->name), param->name.c_str());
			info->module[0]		= '\0';
			info->min_value		= param->from;
			info->max_value		= param->to;
			info->default_value = param->defaultValue;
			return true;
		}

		bool paramsGetValue(const clap_plugin_t *p, clap_id id, double *value) {
			auto *me = self(p);
			if (!me->plugin || id >= me->plugin->getNumParams()) return false;
			*value = me->plugin->getParam(id)->get();
			return true;
		}

		bool paramsValueToText(const clap_plugin_t *p, clap_id id, double value, char *out, uint32_t outSize) {
			auto *me = self(p);
			if (!me->plugin || id >= me->plugin->getNumParams()) return false;
			auto param = me->plugin->getParam(id);
			if (param->type == PluginParameter::Type::Indexed && !param->options.empty()) {
				const int idx = std::clamp((int) std::round(value), 0, (int) param->options.size() - 1);
				snprintf(out, outSize, "%s", param->options[(size_t) idx].c_str());
			} else if (param->type == PluginParameter::Type::Int) {
				snprintf(out, outSize, "%d", (int) std::round(value));
			} else if (!param->unit.empty()) {
				snprintf(out, outSize, "%.2f %s", value, param->unit.c_str());
			} else {
				snprintf(out, outSize, "%.2f", value);
			}
			return true;
		}

		bool paramsTextToValue(const clap_plugin_t *p, clap_id id, const char *text, double *value) {
			if (text == nullptr || value == nullptr) return false;
			*value = atof(text);
			return true;
		}

		void paramsFlush(const clap_plugin_t *p, const clap_input_events_t *in, const clap_output_events_t *out) {
			auto *me = self(p);
			if (in != nullptr) {
				const uint32_t n = in->size(in);
				for (uint32_t i = 0; i < n; ++i) {
					me->handleEvent(in->get(in, i));
				}
			}
			me->drainPending(out);
		}

		const clap_plugin_params_t paramsExt = {
			paramsCount, paramsGetInfo, paramsGetValue, paramsValueToText, paramsTextToValue, paramsFlush};

		// ----- state -----

		// CLAP state layout (fresh format - a CLAP state can't come from the
		// VST3 anyway): "MZCS" magic + u32 version + u32 paramCount + that
		// many f32 values + the engine blob. The explicit parameter block
		// exists because a product's own state may not round-trip its live
		// parameter values (Koala's perform-FX positions aren't part of the
		// song); the magic also makes garbage/empty states loudly invalid.
		constexpr uint32_t kStateMagic	 = 0x4d5a4353; // "MZCS"
		constexpr uint32_t kStateVersion = 1;

		bool writeAll(const clap_ostream_t *stream, const void *data, size_t size) {
			size_t written = 0;
			while (written < size) {
				const int64_t n =
					stream->write(stream, (const uint8_t *) data + written, (uint64_t) (size - written));
				if (n <= 0) return false;
				written += (size_t) n;
			}
			return true;
		}

		// Loop-fill: hosts (and the validator) may hand back tiny reads.
		size_t readAll(const clap_istream_t *stream, void *data, size_t size) {
			size_t got = 0;
			while (got < size) {
				const int64_t n = stream->read(stream, (uint8_t *) data + got, (uint64_t) (size - got));
				if (n <= 0) break;
				got += (size_t) n;
			}
			return got;
		}

		bool stateSave(const clap_plugin_t *p, const clap_ostream_t *stream) {
			auto *me = self(p);
			if (!me->plugin || stream == nullptr) return false;

			const uint32_t header[3] = {kStateMagic, kStateVersion, (uint32_t) me->plugin->getNumParams()};
			if (!writeAll(stream, header, sizeof(header))) return false;
			for (uint32_t i = 0; i < header[2]; ++i) {
				const float v = me->plugin->getParam(i)->get();
				if (!writeAll(stream, &v, sizeof(v))) return false;
			}

			// serializeShared keeps big state off the heap-doubling path
			// (Koala overrides it); written out in chunks.
			auto blob = me->plugin->serializeShared();
			if (!blob) return false;
			return writeAll(stream, blob->data(), blob->size());
		}

		bool stateLoad(const clap_plugin_t *p, const clap_istream_t *stream) {
			auto *me = self(p);
			if (!me->plugin || stream == nullptr) return false;

			uint32_t header[3] = {};
			if (readAll(stream, header, sizeof(header)) != sizeof(header) || header[0] != kStateMagic
				|| header[1] != kStateVersion) {
				return false;
			}
			std::vector<float> paramValues(header[2]);
			if (!paramValues.empty()
				&& readAll(stream, paramValues.data(), paramValues.size() * sizeof(float))
					   != paramValues.size() * sizeof(float)) {
				return false;
			}

			// deserializeStreamed pulls straight from the host's stream (Koala
			// spools to disk in chunks; the default buffers then deserializes).
			me->plugin->deserializeStreamed(
				[stream](void *out, size_t n) -> size_t { return readAll(stream, out, n); });

			// Re-apply the live parameter values on top of the engine state.
			const uint32_t count = std::min<uint32_t>(header[2], (uint32_t) me->plugin->getNumParams());
			for (uint32_t i = 0; i < count; ++i) {
				me->plugin->hostUpdatedParameter(i, paramValues[i]);
			}
			return true;
		}

		const clap_plugin_state_t stateExt = {stateSave, stateLoad};

#if defined(__linux__) && !defined(__ANDROID__)

		// ----- gui -----

		bool guiIsApiSupported(const clap_plugin_t *, const char *api, bool isFloating) {
			return !isFloating && strcmp(api, CLAP_WINDOW_API_X11) == 0;
		}

		bool guiGetPreferredApi(const clap_plugin_t *, const char **api, bool *isFloating) {
			*api		= CLAP_WINDOW_API_X11;
			*isFloating = false;
			return true;
		}

		bool guiCreate(const clap_plugin_t *p, const char *api, bool isFloating) {
			if (!guiIsApiSupported(p, api, isFloating)) return false;
			return self(p)->guiCreate();
		}

		void guiDestroy(const clap_plugin_t *p) {
			self(p)->guiDestroy();
		}

		bool guiSetScale(const clap_plugin_t *p, double scale) {
			auto *me	 = self(p);
			me->guiScale = (float) scale;
			if (me->view && me->view->attached()) me->view->setPixelScale((float) scale);
			return true;
		}

		bool guiGetSize(const clap_plugin_t *p, uint32_t *width, uint32_t *height) {
			auto *me = self(p);
			if (me->guiWidth == 0 || me->guiHeight == 0) {
				const float scale = me->effectiveScale();
				me->guiWidth	  = (uint32_t) std::lround(me->viewCfg.defaultWidth * scale);
				me->guiHeight	  = (uint32_t) std::lround(me->viewCfg.defaultHeight * scale);
			}
			*width	= me->guiWidth;
			*height = me->guiHeight;
			return true;
		}

		bool guiCanResize(const clap_plugin_t *) {
			return true;
		}

		bool guiGetResizeHints(const clap_plugin_t *, clap_gui_resize_hints_t *hints) {
			hints->can_resize_horizontally = true;
			hints->can_resize_vertically   = true;
			hints->preserve_aspect_ratio   = false;
			return true;
		}

		bool guiAdjustSize(const clap_plugin_t *p, uint32_t *width, uint32_t *height) {
			auto *me		  = self(p);
			const auto &view  = me->viewCfg;
			const float scale = me->effectiveScale();
			*width			  = std::clamp(*width,
										   (uint32_t) std::lround(view.minWidth * scale),
										   (uint32_t) std::lround(view.maxWidth * scale));
			*height			  = std::clamp(*height,
										   (uint32_t) std::lround(view.minHeight * scale),
										   (uint32_t) std::lround(view.maxHeight * scale));
			return true;
		}

		bool guiSetSize(const clap_plugin_t *p, uint32_t width, uint32_t height) {
			auto *me	  = self(p);
			me->guiWidth  = width;
			me->guiHeight = height;
			if (me->view) me->view->resize((int) width, (int) height);
			return true;
		}

		bool guiSetParent(const clap_plugin_t *p, const clap_window_t *window) {
			return self(p)->guiSetParent(window);
		}

		bool guiSetTransient(const clap_plugin_t *, const clap_window_t *) {
			return false;
		}

		void guiSuggestTitle(const clap_plugin_t *, const char *) {
		}

		bool guiShow(const clap_plugin_t *) {
			return true;
		}

		bool guiHide(const clap_plugin_t *) {
			return true;
		}

		const clap_plugin_gui_t guiExt = {guiIsApiSupported,
										  guiGetPreferredApi,
										  guiCreate,
										  guiDestroy,
										  guiSetScale,
										  guiGetSize,
										  guiCanResize,
										  guiGetResizeHints,
										  guiAdjustSize,
										  guiSetSize,
										  guiSetParent,
										  guiSetTransient,
										  guiSuggestTitle,
										  guiShow,
										  guiHide};

		// ----- timer / fd pumps -----

		void onTimer(const clap_plugin_t *p, clap_id timerId) {
			auto *me = self(p);
			if (me->view && timerId == me->frameTimerId) me->view->renderFrame();
		}

		const clap_plugin_timer_support_t timerExt = {onTimer};

		void onFd(const clap_plugin_t *p, int fd, clap_posix_fd_flags_t) {
			auto *me = self(p);
			if (me->view && fd == me->registeredFd) me->view->handleXEvents();
		}

		const clap_plugin_posix_fd_support_t fdExt = {onFd};

#endif // __linux__

		// ----- clap_plugin_t -----

		bool pluginInit(const clap_plugin_t *p) {
			return self(p)->init();
		}

		void pluginDestroy(const clap_plugin_t *p) {
			auto *me = self(p);
			me->destroy();
			delete me;
		}

		bool pluginActivate(const clap_plugin_t *p, double sampleRate, uint32_t, uint32_t) {
			auto *me = self(p);
			if (!me->plugin) return false;
			me->plugin->setSampleRate(sampleRate);
			me->plugin->init(2, 2);
			if (me->spec->onActivate) me->spec->onActivate(me->plugin, true);
			return true;
		}

		void pluginDeactivate(const clap_plugin_t *p) {
			auto *me = self(p);
			if (me->plugin && me->spec->onActivate) me->spec->onActivate(me->plugin, false);
		}

		bool pluginStartProcessing(const clap_plugin_t *) {
			return true;
		}

		void pluginStopProcessing(const clap_plugin_t *) {
		}

		void pluginReset(const clap_plugin_t *) {
		}

		clap_process_status pluginProcess(const clap_plugin_t *p, const clap_process_t *process) {
			return self(p)->process(process);
		}

		const void *pluginGetExtension(const clap_plugin_t *, const char *id) {
			if (strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0) return &audioPortsExt;
			if (strcmp(id, CLAP_EXT_NOTE_PORTS) == 0) return &notePortsExt;
			if (strcmp(id, CLAP_EXT_PARAMS) == 0) return &paramsExt;
			if (strcmp(id, CLAP_EXT_STATE) == 0) return &stateExt;
#if defined(__linux__) && !defined(__ANDROID__)
			if (strcmp(id, CLAP_EXT_GUI) == 0) return &guiExt;
			if (strcmp(id, CLAP_EXT_TIMER_SUPPORT) == 0) return &timerExt;
			if (strcmp(id, CLAP_EXT_POSIX_FD_SUPPORT) == 0) return &fdExt;
#endif
			return nullptr;
		}

		void pluginOnMainThread(const clap_plugin_t *) {
		}

		// ----- descriptor / factory / entry -----

		// Built once from the spec; the features array needs a null terminator.
		const clap_plugin_descriptor_t &descriptor() {
			static std::vector<const char *> features = [] {
				auto f = mzglClapSpec().features;
				f.push_back(nullptr);
				return f;
			}();
			static const clap_plugin_descriptor_t desc = [] {
				const ClapSpec &spec = mzglClapSpec();
				clap_plugin_descriptor_t d {};
				d.clap_version = CLAP_VERSION_INIT;
				d.id		   = spec.id;
				d.name		   = spec.name;
				d.vendor	   = spec.vendor;
				d.url		   = spec.url ? spec.url : "";
				d.manual_url   = "";
				d.support_url  = "";
				d.version	   = spec.version;
				d.description  = spec.description ? spec.description : "";
				d.features	   = features.data();
				return d;
			}();
			return desc;
		}

		uint32_t factoryGetPluginCount(const clap_plugin_factory_t *) {
			return 1;
		}

		const clap_plugin_descriptor_t *factoryGetPluginDescriptor(const clap_plugin_factory_t *, uint32_t index) {
			return index == 0 ? &descriptor() : nullptr;
		}

		const clap_plugin_t *
			factoryCreatePlugin(const clap_plugin_factory_t *, const clap_host_t *host, const char *pluginId) {
			if (host == nullptr || !clap_version_is_compatible(host->clap_version)) return nullptr;
			if (pluginId == nullptr || strcmp(pluginId, descriptor().id) != 0) return nullptr;

			auto *me						= new MzglClapPlugin();
			me->host						= host;
			me->spec						= &mzglClapSpec();
			me->clapPlugin.desc				= &descriptor();
			me->clapPlugin.plugin_data		= me;
			me->clapPlugin.init				= pluginInit;
			me->clapPlugin.destroy			= pluginDestroy;
			me->clapPlugin.activate			= pluginActivate;
			me->clapPlugin.deactivate		= pluginDeactivate;
			me->clapPlugin.start_processing = pluginStartProcessing;
			me->clapPlugin.stop_processing	= pluginStopProcessing;
			me->clapPlugin.reset			= pluginReset;
			me->clapPlugin.process			= pluginProcess;
			me->clapPlugin.get_extension	= pluginGetExtension;
			me->clapPlugin.on_main_thread	= pluginOnMainThread;
			return &me->clapPlugin;
		}

		const clap_plugin_factory_t pluginFactory = {
			factoryGetPluginCount, factoryGetPluginDescriptor, factoryCreatePlugin};

		bool entryInit(const char *) {
			return true;
		}

		void entryDeinit() {
		}

		const void *entryGetFactory(const char *factoryId) {
			return strcmp(factoryId, CLAP_PLUGIN_FACTORY_ID) == 0 ? &pluginFactory : nullptr;
		}

	} // namespace

} // namespace mzglclap

extern "C" CLAP_EXPORT const clap_plugin_entry_t clap_entry = {
	CLAP_VERSION_INIT,
	mzglclap::entryInit,
	mzglclap::entryDeinit,
	mzglclap::entryGetFactory,
};

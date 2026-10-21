// Linux IPlugView: a thin adapter between the VST3 view protocol and the
// shared X11/EGL embedding core (plugin/linux/X11PluginView). The host hands
// us an X11 window id (kPlatformTypeX11EmbedWindowID) and - because Linux has
// no global run loop - a Steinberg::Linux::IRunLoop on its IPlugFrame, which
// drives both input (an event handler on the X connection's fd) and rendering
// (a ~16ms timer), mirroring the Windows view's WM_TIMER cadence.
#if defined(__linux__) && !defined(__ANDROID__)

#	include "MzglVST3View.h"

#	include "X11PluginView.h"
#	include "Plugin.h"
#	include "filesystem.h"
#	include "log.h"
#	include "util.h"

#	include <dlfcn.h>

#	include <cstring>

using namespace Steinberg;

namespace mzglvst {

	namespace {
		constexpr Linux::TimerInterval kFrameIntervalMs = 16;

		// The bundle layout is <Name>.vst3/Contents/<arch>-linux/<Name>.so with
		// data in <Name>.vst3/Contents/Resources/data. libmzgl was built without
		// MZGL_PLUGIN_VST, so its baked-in dataPath() resolves relative to the
		// host's executable - the view passes this to X11PluginView::attach to
		// override it (same job as the mac/win views' bundle-path overrides).
		std::string bundleDataDir() {
			Dl_info info;
			if (dladdr((void *) &bundleDataDir, &info) && info.dli_fname) {
				return (fs::path(info.dli_fname).parent_path().parent_path() / "Resources" / "data").string();
			}
			return {};
		}
	} // namespace

	struct MzglVST3View::Impl
		: public FObject
		, public Linux::IEventHandler
		, public Linux::ITimerHandler {
		mzgl::X11PluginView view;
		IPtr<Linux::IRunLoop> runLoop;
		bool handlersRegistered {false};

		OBJ_METHODS(Impl, FObject)
		REFCOUNT_METHODS(FObject)
		DEF_INTERFACES_2(Linux::IEventHandler, Linux::ITimerHandler, FObject)

		void PLUGIN_API onFDIsSet(Linux::FileDescriptor) SMTG_OVERRIDE { view.handleXEvents(); }
		void PLUGIN_API onTimer() SMTG_OVERRIDE { view.renderFrame(); }

		void unregisterHandlers() {
			if (runLoop && handlersRegistered) {
				runLoop->unregisterEventHandler(this);
				runLoop->unregisterTimer(this);
				handlersRegistered = false;
			}
		}

		void teardown() {
			unregisterHandlers();
			view.detach();
			runLoop = nullptr;
		}
	};

	MzglVST3View::MzglVST3View(MzglVST3PluginProvider *provider, const ViewConfig &_config)
		: impl(NEW Impl())
		, provider(provider)
		, config(_config) {
		viewRect = {0, 0, config.defaultWidth, config.defaultHeight};
	}

	MzglVST3View::~MzglVST3View() {
		if (impl) impl->teardown();
	}

	tresult PLUGIN_API MzglVST3View::queryInterface(const TUID iid, void **obj) {
		QUERY_INTERFACE(iid, obj, FUnknown::iid, IPlugView)
		QUERY_INTERFACE(iid, obj, IPlugView::iid, IPlugView)
		QUERY_INTERFACE(iid, obj, IPlugViewContentScaleSupport::iid, IPlugViewContentScaleSupport)
		*obj = nullptr;
		return kNoInterface;
	}

	tresult PLUGIN_API MzglVST3View::isPlatformTypeSupported(FIDString type) {
		if (type && strcmp(type, kPlatformTypeX11EmbedWindowID) == 0) {
			return kResultTrue;
		}
		return kResultFalse;
	}

	ViewRect MzglVST3View::scaledRect(int logicalW, int logicalH) const {
		return {0, 0, (int32) (logicalW * contentScale + 0.5f), (int32) (logicalH * contentScale + 0.5f)};
	}

	tresult PLUGIN_API MzglVST3View::attached(void *parent, FIDString type) {
		if (!parent || !type) return kInvalidArgument;
		if (strcmp(type, kPlatformTypeX11EmbedWindowID) != 0) return kResultFalse;

		// No run loop, no editor: on Linux rendering and input are entirely
		// host-driven, so a host that doesn't provide IRunLoop can't show a view
		// (the DSP side keeps working without one).
		if (!impl->runLoop && plugFrame != nullptr) {
			Linux::IRunLoop *runLoop = nullptr;
			if (plugFrame->queryInterface(Linux::IRunLoop::iid, (void **) &runLoop) == kResultOk
				&& runLoop != nullptr) {
				impl->runLoop = owned(runLoop);
			}
		}
		if (!impl->runLoop) {
			Log::e() << "MzglVST3View: host provides no IRunLoop - no editor";
			return kResultFalse;
		}

		// A host that never told us its scale: read the display's, and let the
		// host know our preferred size at that scale below.
		bool sizeChanged = false;
		if (!hostSetContentScale) {
			const float scale = mzgl::X11PluginView::detectPixelScale();
			if (scale != contentScale) {
				contentScale = scale;
				viewRect	 = scaledRect(config.defaultWidth, config.defaultHeight);
				sizeChanged	 = true;
			}
		}

		mzgl::X11PluginView::Setup setup;
		setup.widthPx	 = viewRect.getWidth();
		setup.heightPx	 = viewRect.getHeight();
		setup.pixelScale = contentScale;
		setup.dataDir	 = bundleDataDir();
		setup.getPlugin	 = [this]() -> std::shared_ptr<Plugin> {
			return provider ? provider->getPlugin() : nullptr;
		};
		// The window id arrives as the pointer-sized value of an X11 Window.
		if (!impl->view.attach((uint64_t) (uintptr_t) parent, setup)) {
			return kResultFalse;
		}

		impl->runLoop->registerEventHandler(impl.get(), impl->view.connectionFd());
		impl->runLoop->registerTimer(impl.get(), kFrameIntervalMs);
		impl->handlersRegistered = true;

		if (sizeChanged && plugFrame != nullptr) {
			plugFrame->resizeView(this, &viewRect);
		}
		return kResultTrue;
	}

	tresult PLUGIN_API MzglVST3View::removed() {
		impl->teardown();
		return kResultTrue;
	}

	void MzglVST3View::renderFrame() {
		impl->view.renderFrame();
	}

	tresult PLUGIN_API MzglVST3View::getSize(ViewRect *size) {
		if (!size) return kInvalidArgument;
		*size = viewRect;
		return kResultTrue;
	}

	tresult PLUGIN_API MzglVST3View::onSize(ViewRect *newSize) {
		if (!newSize) return kInvalidArgument;
		viewRect = *newSize;
		impl->view.resize(viewRect.getWidth(), viewRect.getHeight());
		return kResultTrue;
	}

	tresult PLUGIN_API MzglVST3View::setFrame(IPlugFrame *frame) {
		plugFrame = frame;
		return kResultTrue;
	}

	tresult PLUGIN_API MzglVST3View::checkSizeConstraint(ViewRect *rect) {
		if (!rect) return kInvalidArgument;
		int width  = rect->right - rect->left;
		int height = rect->bottom - rect->top;

		const ViewRect minRect = scaledRect(config.minWidth, config.minHeight);
		const ViewRect maxRect = scaledRect(config.maxWidth, config.maxHeight);
		if (width < minRect.getWidth()) width = minRect.getWidth();
		if (width > maxRect.getWidth()) width = maxRect.getWidth();
		if (height < minRect.getHeight()) height = minRect.getHeight();
		if (height > maxRect.getHeight()) height = maxRect.getHeight();

		rect->right	 = rect->left + width;
		rect->bottom = rect->top + height;
		return kResultTrue;
	}

	tresult PLUGIN_API MzglVST3View::setContentScaleFactor(ScaleFactor factor) {
		if (factor <= 0.f) return kInvalidArgument;
		const float oldScale = contentScale;
		contentScale		 = factor;
		hostSetContentScale	 = true;
		if (!impl->view.attached()) {
			// Before attached(): just our preferred size, which getSize() reports.
			viewRect = scaledRect(config.defaultWidth, config.defaultHeight);
			return kResultTrue;
		}
		// Live scale change: redraw at the new scale and ask the host for the
		// equivalent size.
		impl->view.setPixelScale(contentScale);
		if (plugFrame != nullptr && oldScale > 0.f) {
			ViewRect wanted = scaledRect((int) (viewRect.getWidth() / oldScale + 0.5f),
										 (int) (viewRect.getHeight() / oldScale + 0.5f));
			plugFrame->resizeView(this, &wanted);
		}
		return kResultTrue;
	}

} // namespace mzglvst

#endif // __linux__ && !__ANDROID__

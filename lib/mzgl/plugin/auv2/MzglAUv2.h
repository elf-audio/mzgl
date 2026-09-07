#pragma once

// Generic AUv2 (.component) wrappers around an mzgl ::Plugin, built on Apple's
// AudioUnitSDK. The AUv2 siblings of MzglVST3SingleComponent and of mzgl's AUv3
// MZGLEffectAU: one ::Plugin instance is shared by the audio unit and its editor.
//
//   MzglAUv2Effect      aufx / aumf  - ausdk::AUMIDIEffectBase: stereo in, stereo out,
//                                      optional MIDI in
//   MzglAUv2Instrument  aumu         - ausdk::MusicDeviceBase: no audio input, MIDI in,
//                                      one stereo output bus per Plugin::getNumOutputBusses()
//
// Per plugin you write a tiny subclass that supplies the ::Plugin and a Config,
// and declare the component entry point with the SDK macro:
//
//   class GainAUv2 : public mzglau::MzglAUv2Effect {
//   public:
//       explicit GainAUv2(AudioComponentInstance ci)
//           : MzglAUv2Effect(ci, std::make_shared<GainPlugin>(), config()) {}
//   };
//   AUSDK_COMPONENT_ENTRY(ausdk::AUBaseFactory, GainAUv2)   // -> GainAUv2Factory
//   (AUBaseFactory for aufx; AUMIDIEffectFactory for aumf so hosts can send MIDI;
//    AUMusicDeviceFactory for an MzglAUv2Instrument / aumu)
//
// One .component can publish several AudioComponents (e.g. an aumu and an aumf
// around the same ::Plugin): give each its own subclass + AUSDK_COMPONENT_ENTRY,
// list every factory function in the Info.plist and export them all
// (mzgl_auv2_configure_bundle: FACTORY_FUNCTION + EXTRA_FACTORY_FUNCTIONS).
//
// The Cocoa editor lives in MzglAUv2View.mm; its ObjC class names are prefixed
// with MZGL_AUV2_CLASS_PREFIX (set per target by mzgl_add_auv2_plugin) so two of
// our components loaded into one host don't clash.

#include <AudioUnitSDK/AUMIDIEffectBase.h>
#include <AudioUnitSDK/MusicDeviceBase.h>

// Realtime-safety annotation introduced in AudioUnitSDK 1.4 (which needs C++23);
// we build against 1.3, where the overrides are plain.
#ifndef AUSDK_RTSAFE
#	define AUSDK_RTSAFE
#endif

#include <CoreFoundation/CoreFoundation.h>
#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include "FloatBuffer.h"

class Plugin;

namespace mzglau {

struct AUv2Config {
	bool midiInput = false; // forward MIDI events to Plugin::midiReceivedAtTime
	// Editor size in points and the range the host may resize it within.
	int viewWidth  = 420;
	int viewHeight = 480;
	int minWidth   = 240;
	int minHeight  = 240;
	int maxWidth   = 4096;
	int maxHeight  = 4096;
};

// Custom property (>= 64000 per Apple) through which the Cocoa view factory,
// running in the host process, fetches the C++ audio unit it belongs to.
constexpr AudioUnitPropertyID kMzglAUv2Property_Instance = 64001;

// What the Cocoa view needs from the audio unit, whatever its AU base class.
class MzglAUv2Host {
public:
	virtual ~MzglAUv2Host()										= default;
	virtual std::shared_ptr<::Plugin> getPlugin() const		= 0;
	virtual const AUv2Config &getConfig() const				= 0;
};

/**
 * Everything the two wrappers share: parameters both ways, ClassInfo state via
 * Plugin::serialize, factory presets from the mzgl PresetManager, transport,
 * MIDI, the Cocoa UI property and the interleave/deinterleave plumbing.
 * AUBaseT is ausdk::AUMIDIEffectBase or ausdk::MusicDeviceBase (both AUBase +
 * AUMIDIBase); the subclasses below add the format-specific render entry point.
 */
template <class AUBaseT>
class MzglAUv2Unit
	: public AUBaseT
	, public MzglAUv2Host {
public:
	~MzglAUv2Unit() override;

	std::shared_ptr<::Plugin> getPlugin() const override { return plugin; }
	const AUv2Config &getConfig() const override { return config; }

	// AUBase
	OSStatus Initialize() override;
	void Cleanup() override;

	OSStatus GetPropertyInfo(AudioUnitPropertyID inID,
							 AudioUnitScope inScope,
							 AudioUnitElement inElement,
							 UInt32 &outDataSize,
							 bool &outWritable) override;
	OSStatus GetProperty(AudioUnitPropertyID inID,
						 AudioUnitScope inScope,
						 AudioUnitElement inElement,
						 void *outData) override;

	OSStatus GetParameterInfo(AudioUnitScope inScope,
							  AudioUnitParameterID inParameterID,
							  AudioUnitParameterInfo &outParameterInfo) override;
	OSStatus GetParameterValueStrings(AudioUnitScope inScope,
									  AudioUnitParameterID inParameterID,
									  CFArrayRef *outStrings) override;
	OSStatus SetParameter(AudioUnitParameterID inID,
						  AudioUnitScope inScope,
						  AudioUnitElement inElement,
						  AudioUnitParameterValue inValue,
						  UInt32 inBufferOffsetInFrames) override;

	OSStatus SaveState(CFPropertyListRef *outData) override;
	OSStatus RestoreState(CFPropertyListRef plist) override;

	OSStatus GetPresets(CFArrayRef *outData) const override;
	OSStatus NewFactoryPresetSet(const AUPreset &inNewFactoryPreset) override;

	OSStatus ChangeStreamFormat(AudioUnitScope inScope,
								AudioUnitElement inElement,
								const AudioStreamBasicDescription &inPrevFormat,
								const AudioStreamBasicDescription &inNewFormat) override;

protected:
	// BaseArgs are forwarded to AUBaseT's constructor.
	template <class... BaseArgs>
	MzglAUv2Unit(std::shared_ptr<::Plugin> plugin, const AUv2Config &config, BaseArgs &&...baseArgs);

	// AUMIDIBase
	OSStatus HandleMIDIEvent(UInt8 inStatus, UInt8 inChannel, UInt8 inData1, UInt8 inData2, UInt32 inStartFrame)
		AUSDK_RTSAFE override;

	// --- render plumbing for the subclasses (audio thread) ---
	// Sync host parameters + transport into the plugin at the start of a block.
	void beginRender();
	// Fill interleavedIn from a (de)interleaved stereo/mono buffer list, or with
	// silence when in == nullptr.
	void interleaveInput(const AudioBufferList *in, UInt32 frames);
	// Run the plugin: interleavedIn -> interleavedOuts (one per output bus).
	void runPlugin(UInt32 frames);
	// Write output bus `bus` into a host buffer list (planar or interleaved,
	// stereo or mono). Busses the plugin doesn't have come out silent.
	void deinterleaveOutput(AudioBufferList &out, int bus, UInt32 frames) const;

	std::shared_ptr<::Plugin> plugin;
	AUv2Config config;

private:
	void setup();
	// Push every host-side parameter value that differs from what the plugin
	// last saw into the plugin (covers scheduled/ramped automation too).
	void pullHostParameters();
	// Called by the plugin (UI thread) when a knob moves: update the AU's value
	// and tell listeners (host automation / generic views).
	void pluginParameterChanged(unsigned int index, float value);
	void mirrorPluginParameters();
	void applyPreset(int index);
	void setDataPathToBundle();

	std::unique_ptr<std::atomic<float>[]> lastHostValues;
	std::vector<std::string> factoryPresetNames;
	std::vector<AUPreset> factoryPresetStorage;
	CFArrayRef factoryPresets {nullptr};

	FloatBuffer interleavedIn;
	std::vector<FloatBuffer> interleavedOuts;
};

/** aufx / aumf: stereo in -> stereo out (first output bus), optional MIDI in. */
class MzglAUv2Effect : public MzglAUv2Unit<ausdk::AUMIDIEffectBase> {
public:
	MzglAUv2Effect(AudioComponentInstance ci, std::shared_ptr<::Plugin> plugin, const AUv2Config &config);

	UInt32 SupportedNumChannels(const AUChannelInfo **outInfo) override;
	OSStatus ProcessBufferLists(AudioUnitRenderActionFlags &ioActionFlags,
								const AudioBufferList &inBuffer,
								AudioBufferList &outBuffer,
								UInt32 inFramesToProcess) AUSDK_RTSAFE override;

	bool SupportsTail() AUSDK_RTSAFE override { return true; }
	Float64 GetTailTime() AUSDK_RTSAFE override { return 0.0; }
};

/**
 * aumu: no audio input, MIDI in, Plugin::getNumOutputBusses() stereo output
 * busses (hosts like Logic expose the extra busses as aux channels).
 */
class MzglAUv2Instrument : public MzglAUv2Unit<ausdk::MusicDeviceBase> {
public:
	MzglAUv2Instrument(AudioComponentInstance ci, std::shared_ptr<::Plugin> plugin, const AUv2Config &config);

	UInt32 SupportedNumChannels(const AUChannelInfo **outInfo) override;
	bool StreamFormatWritable(AudioUnitScope scope, AudioUnitElement element) override;
	bool CanScheduleParameters() const override { return false; }
	OSStatus Render(AudioUnitRenderActionFlags &ioActionFlags,
					const AudioTimeStamp &inTimeStamp,
					UInt32 inNumberFrames) override;

private:
	static UInt32 numOutputBusses(const std::shared_ptr<::Plugin> &plugin);
};

} // namespace mzglau

#include "MzglAUv2.h"
#include "MzglAUv2View.h"

#include "Plugin.h"
#include "PluginParameter.h"
#include "MidiMessage.h"
#include "util.h"
#include "filesystem.h"
#include "log.h"

#include <AudioUnitSDK/AUUtility.h>
#include <AudioToolbox/AudioUnitUtilities.h>
#include <dlfcn.h>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace mzglau {

namespace {
	const CFStringRef kStateKey = CFSTR("mzgl-plugin-state");

	// Where our .component lives: <Plugin>.component/Contents/MacOS/<Plugin> -> bundle dir.
	fs::path componentBundlePath() {
		Dl_info info;
		if (!dladdr((void *) &componentBundlePath, &info) || info.dli_fname == nullptr) return {};
		return fs::path(info.dli_fname).parent_path().parent_path().parent_path();
	}
} // namespace

// ---------------------------------------------------------------------------
// MzglAUv2Unit<AUBaseT>
// ---------------------------------------------------------------------------
template <class AUBaseT>
template <class... BaseArgs>
MzglAUv2Unit<AUBaseT>::MzglAUv2Unit(std::shared_ptr<::Plugin> _plugin, const AUv2Config &_config, BaseArgs &&...baseArgs)
	: AUBaseT(std::forward<BaseArgs>(baseArgs)...)
	, plugin(std::move(_plugin))
	, config(_config) {
	setup();
}

template <class AUBaseT>
void MzglAUv2Unit<AUBaseT>::setup() {
	setDataPathToBundle();

	const auto numParams = static_cast<UInt32>(plugin->getNumParams());
	this->Globals()->UseIndexedParameters(numParams);
	lastHostValues = std::make_unique<std::atomic<float>[]>(numParams);
	for (UInt32 i = 0; i < numParams; ++i) {
		const float v = plugin->getParam(i)->get();
		this->Globals()->SetParameter(i, v);
		lastHostValues[i].store(v);
	}

	plugin->sendUpdatedParameterToHost = [this](unsigned int i, float v) { pluginParameterChanged(i, v); };

	// Factory presets, if the plugin ships any (mzgl PresetManager: files in
	// data/factory-presets named <identifier>preset).
	factoryPresetNames = plugin->getPresetManager()->getFactoryPresetNames();
	if (!factoryPresetNames.empty()) {
		// kAudioUnitProperty_FactoryPresets is a CFArray of AUPreset* (hosts cast
		// the elements); the structs live in factoryPresetStorage for the life
		// of the audio unit, so the array carries no value callbacks.
		factoryPresetStorage.resize(factoryPresetNames.size());
		std::vector<const void *> values;
		for (size_t i = 0; i < factoryPresetNames.size(); ++i) {
			factoryPresetStorage[i].presetNumber = static_cast<SInt32>(i);
			factoryPresetStorage[i].presetName =
				CFStringCreateWithCString(nullptr, factoryPresetNames[i].c_str(), kCFStringEncodingUTF8);
			values.push_back(&factoryPresetStorage[i]);
		}
		factoryPresets = CFArrayCreate(nullptr, values.data(), static_cast<CFIndex>(values.size()), nullptr);
		// Advertise preset 0 as current, like the AUv3 does (state stays at the
		// plugin's defaults until a host actually selects a preset).
		this->SetAFactoryPresetAsCurrent(factoryPresetStorage[0]);
	}

	interleavedIn.reserve(8192 * 2);
}

template <class AUBaseT>
MzglAUv2Unit<AUBaseT>::~MzglAUv2Unit() {
	if (plugin) plugin->sendUpdatedParameterToHost = nullptr;
	if (factoryPresets) CFRelease(factoryPresets);
	for (auto &p: factoryPresetStorage) {
		if (p.presetName) CFRelease(p.presetName);
	}
}

template <class AUBaseT>
void MzglAUv2Unit<AUBaseT>::setDataPathToBundle() {
	// libmzgl's dataPath() knows nothing about component bundles; point it at
	// our Contents/Resources/data (same trick as MzglVST3View.mm).
	const fs::path bundle = componentBundlePath();
	if (bundle.empty()) return;
	setDataPath((bundle / "Contents" / "Resources" / "data").string());
}

// ---------------------------------------------------------------------------
// Lifecycle / format
// ---------------------------------------------------------------------------
template <class AUBaseT>
OSStatus MzglAUv2Unit<AUBaseT>::Initialize() {
	const OSStatus result = AUBaseT::Initialize();
	if (result != noErr) return result;
	// (AUEffectBase has GetSampleRate(); AUBase itself doesn't, so read the output format.)
	plugin->setSampleRate(this->Output(0).GetStreamFormat().mSampleRate);
	plugin->init(2, 2);
	return noErr;
}

template <class AUBaseT>
void MzglAUv2Unit<AUBaseT>::Cleanup() {
	AUBaseT::Cleanup();
}

template <class AUBaseT>
OSStatus MzglAUv2Unit<AUBaseT>::ChangeStreamFormat(AudioUnitScope inScope,
												   AudioUnitElement inElement,
												   const AudioStreamBasicDescription &inPrevFormat,
												   const AudioStreamBasicDescription &inNewFormat) {
	const OSStatus result = AUBaseT::ChangeStreamFormat(inScope, inElement, inPrevFormat, inNewFormat);
	if (result == noErr && inScope == kAudioUnitScope_Output) {
		plugin->setSampleRate(inNewFormat.mSampleRate);
	}
	return result;
}

// ---------------------------------------------------------------------------
// Properties
// ---------------------------------------------------------------------------
template <class AUBaseT>
OSStatus MzglAUv2Unit<AUBaseT>::GetPropertyInfo(AudioUnitPropertyID inID,
												AudioUnitScope inScope,
												AudioUnitElement inElement,
												UInt32 &outDataSize,
												bool &outWritable) {
	if (inScope == kAudioUnitScope_Global) {
		switch (inID) {
			case kAudioUnitProperty_CocoaUI:
				outDataSize = sizeof(AudioUnitCocoaViewInfo);
				outWritable = false;
				return noErr;
			case kMzglAUv2Property_Instance:
				outDataSize = sizeof(void *);
				outWritable = false;
				return noErr;
			default:
				break;
		}
	}
	return AUBaseT::GetPropertyInfo(inID, inScope, inElement, outDataSize, outWritable);
}

template <class AUBaseT>
OSStatus MzglAUv2Unit<AUBaseT>::GetProperty(AudioUnitPropertyID inID,
											AudioUnitScope inScope,
											AudioUnitElement inElement,
											void *outData) {
	if (inScope == kAudioUnitScope_Global) {
		switch (inID) {
			case kAudioUnitProperty_CocoaUI: {
				const fs::path bundle = componentBundlePath();
				if (bundle.empty()) return kAudioUnitErr_InvalidProperty;
				CFURLRef url = CFURLCreateFromFileSystemRepresentation(
					nullptr, reinterpret_cast<const UInt8 *>(bundle.c_str()), static_cast<CFIndex>(bundle.string().size()),
					true);
				auto *info						 = static_cast<AudioUnitCocoaViewInfo *>(outData);
				info->mCocoaAUViewBundleLocation = url;
				info->mCocoaAUViewClass[0] =
					CFStringCreateWithCString(nullptr, MZGL_AUV2_VIEW_FACTORY_CLASS_NAME, kCFStringEncodingUTF8);
				return noErr;
			}
			case kMzglAUv2Property_Instance:
				*static_cast<MzglAUv2Host **>(outData) = this;
				return noErr;
			default:
				break;
		}
	}
	return AUBaseT::GetProperty(inID, inScope, inElement, outData);
}

// ---------------------------------------------------------------------------
// Parameters
// ---------------------------------------------------------------------------
template <class AUBaseT>
OSStatus MzglAUv2Unit<AUBaseT>::GetParameterInfo(AudioUnitScope inScope,
												 AudioUnitParameterID inParameterID,
												 AudioUnitParameterInfo &outParameterInfo) {
	if (inScope != kAudioUnitScope_Global || inParameterID >= plugin->getNumParams()) {
		return kAudioUnitErr_InvalidParameter;
	}
	auto p = plugin->getParam(inParameterID);

	outParameterInfo.flags = kAudioUnitParameterFlag_IsWritable | kAudioUnitParameterFlag_IsReadable
							 | kAudioUnitParameterFlag_CanRamp;
	ausdk::AUBase::FillInParameterName(
		outParameterInfo, CFStringCreateWithCString(nullptr, p->name.c_str(), kCFStringEncodingUTF8), true);
	outParameterInfo.minValue	  = p->from;
	outParameterInfo.maxValue	  = p->to;
	outParameterInfo.defaultValue = p->defaultValue;
	outParameterInfo.unit		  = kAudioUnitParameterUnit_Generic;
	switch (p->type) {
		case PluginParameter::Type::Indexed:
			outParameterInfo.unit = kAudioUnitParameterUnit_Indexed;
			outParameterInfo.flags |= kAudioUnitParameterFlag_ValuesHaveStrings;
			break;
		case PluginParameter::Type::Int:
			outParameterInfo.unit = (p->from == 0.f && p->to == 1.f) ? kAudioUnitParameterUnit_Boolean
																	 : kAudioUnitParameterUnit_Indexed;
			break;
		default:
			if (p->unit == "dB") outParameterInfo.unit = kAudioUnitParameterUnit_Decibels;
			else if (p->unit == "Hz") outParameterInfo.unit = kAudioUnitParameterUnit_Hertz;
			else if (p->unit == "ms") outParameterInfo.unit = kAudioUnitParameterUnit_Milliseconds;
			else if (p->unit == "s") outParameterInfo.unit = kAudioUnitParameterUnit_Seconds;
			else if (p->unit == "%") outParameterInfo.unit = kAudioUnitParameterUnit_Percent;
			else if (!p->unit.empty()) {
				outParameterInfo.unit	  = kAudioUnitParameterUnit_CustomUnit;
				outParameterInfo.unitName = CFStringCreateWithCString(nullptr, p->unit.c_str(), kCFStringEncodingUTF8);
			}
			break;
	}
	return noErr;
}

template <class AUBaseT>
OSStatus MzglAUv2Unit<AUBaseT>::GetParameterValueStrings(AudioUnitScope inScope,
														 AudioUnitParameterID inParameterID,
														 CFArrayRef *outStrings) {
	if (inScope != kAudioUnitScope_Global || inParameterID >= plugin->getNumParams()) {
		return kAudioUnitErr_InvalidParameter;
	}
	auto p = plugin->getParam(inParameterID);
	if (p->type != PluginParameter::Type::Indexed || p->options.empty()) return kAudioUnitErr_InvalidProperty;
	if (outStrings == nullptr) return noErr;
	CFMutableArrayRef arr = CFArrayCreateMutable(nullptr, 0, &kCFTypeArrayCallBacks);
	for (auto &o: p->options) {
		CFStringRef s = CFStringCreateWithCString(nullptr, o.c_str(), kCFStringEncodingUTF8);
		CFArrayAppendValue(arr, s);
		CFRelease(s);
	}
	*outStrings = arr;
	return noErr;
}

template <class AUBaseT>
OSStatus MzglAUv2Unit<AUBaseT>::SetParameter(AudioUnitParameterID inID,
											 AudioUnitScope inScope,
											 AudioUnitElement inElement,
											 AudioUnitParameterValue inValue,
											 UInt32 inBufferOffsetInFrames) {
	const OSStatus result = AUBaseT::SetParameter(inID, inScope, inElement, inValue, inBufferOffsetInFrames);
	if (result == noErr && inScope == kAudioUnitScope_Global && inID < plugin->getNumParams()) {
		lastHostValues[inID].store(inValue);
		plugin->hostUpdatedParameter(inID, inValue);
	}
	return result;
}

template <class AUBaseT>
void MzglAUv2Unit<AUBaseT>::pullHostParameters() {
	const auto n = static_cast<UInt32>(plugin->getNumParams());
	for (UInt32 i = 0; i < n; ++i) {
		const float v = this->Globals()->GetParameter(i);
		if (v != lastHostValues[i].load()) {
			lastHostValues[i].store(v);
			plugin->hostUpdatedParameter(i, v);
		}
	}
}

template <class AUBaseT>
void MzglAUv2Unit<AUBaseT>::pluginParameterChanged(unsigned int index, float value) {
	if (index >= plugin->getNumParams()) return;
	lastHostValues[index].store(value);
	this->Globals()->SetParameter(index, value);

	AudioUnitEvent event {};
	event.mEventType						= kAudioUnitEvent_ParameterValueChange;
	event.mArgument.mParameter.mAudioUnit	= this->GetComponentInstance();
	event.mArgument.mParameter.mParameterID = index;
	event.mArgument.mParameter.mScope		= kAudioUnitScope_Global;
	event.mArgument.mParameter.mElement		= 0;
	AUEventListenerNotify(nullptr, nullptr, &event);
}

// The plugin's parameters may have moved (state restore, preset): mirror them
// into the AU's parameter cache.
template <class AUBaseT>
void MzglAUv2Unit<AUBaseT>::mirrorPluginParameters() {
	for (UInt32 i = 0; i < plugin->getNumParams(); ++i) {
		const float v = plugin->getParam(i)->get();
		this->Globals()->SetParameter(i, v);
		lastHostValues[i].store(v);
	}
}

// ---------------------------------------------------------------------------
// State / presets
// ---------------------------------------------------------------------------
template <class AUBaseT>
OSStatus MzglAUv2Unit<AUBaseT>::SaveState(CFPropertyListRef *outData) {
	const OSStatus result = AUBaseT::SaveState(outData);
	if (result != noErr) return result;
	std::vector<uint8_t> blob;
	plugin->serialize(blob);
	auto dict	   = const_cast<CFMutableDictionaryRef>(static_cast<CFDictionaryRef>(*outData));
	CFDataRef data = CFDataCreate(nullptr, blob.data(), static_cast<CFIndex>(blob.size()));
	CFDictionarySetValue(dict, kStateKey, data);
	CFRelease(data);
	return noErr;
}

template <class AUBaseT>
OSStatus MzglAUv2Unit<AUBaseT>::RestoreState(CFPropertyListRef plist) {
	const OSStatus result = AUBaseT::RestoreState(plist);
	if (result != noErr) return result;
	auto dict = static_cast<CFDictionaryRef>(plist);
	auto data = static_cast<CFDataRef>(CFDictionaryGetValue(dict, kStateKey));
	if (data != nullptr && CFGetTypeID(data) == CFDataGetTypeID()) {
		const auto *bytes = CFDataGetBytePtr(data);
		std::vector<uint8_t> blob(bytes, bytes + CFDataGetLength(data));
		plugin->deserialize(blob);
	}
	mirrorPluginParameters();
	return noErr;
}

template <class AUBaseT>
OSStatus MzglAUv2Unit<AUBaseT>::GetPresets(CFArrayRef *outData) const {
	if (factoryPresets == nullptr) return kAudioUnitErr_InvalidProperty;
	if (outData != nullptr) {
		*outData = factoryPresets;
		CFRetain(factoryPresets);
	}
	return noErr;
}

template <class AUBaseT>
OSStatus MzglAUv2Unit<AUBaseT>::NewFactoryPresetSet(const AUPreset &inNewFactoryPreset) {
	const auto idx = inNewFactoryPreset.presetNumber;
	if (idx < 0 || static_cast<size_t>(idx) >= factoryPresetNames.size()) return kAudioUnitErr_InvalidPropertyValue;
	applyPreset(idx);
	this->SetAFactoryPresetAsCurrent(inNewFactoryPreset);
	return noErr;
}

template <class AUBaseT>
void MzglAUv2Unit<AUBaseT>::applyPreset(int index) {
	plugin->getPresetManager()->loadFactoryPreset(index);
	mirrorPluginParameters();
	this->PropertyChanged(kAudioUnitProperty_PresentPreset, kAudioUnitScope_Global, 0);
}

// ---------------------------------------------------------------------------
// MIDI
// ---------------------------------------------------------------------------
template <class AUBaseT>
OSStatus MzglAUv2Unit<AUBaseT>::HandleMIDIEvent(UInt8 inStatus, UInt8 inChannel, UInt8 inData1, UInt8 inData2, UInt32 inStartFrame)
	AUSDK_RTSAFE {
	if (!config.midiInput) return noErr;
	plugin->midiReceivedAtTime(MidiMessage {static_cast<uint8_t>(inStatus | (inChannel & 0x0f)), inData1, inData2},
							   inStartFrame);
	return noErr;
}

// ---------------------------------------------------------------------------
// Render plumbing
// ---------------------------------------------------------------------------
template <class AUBaseT>
void MzglAUv2Unit<AUBaseT>::beginRender() {
	plugin->hasStarted = true;
	pullHostParameters();

	Float64 beat = 0, tempo = 0;
	if (this->CallHostBeatAndTempo(&beat, &tempo) == noErr) {
		if (tempo > 0) plugin->bpm = tempo;
		plugin->beatPosition = beat;
	}
	Boolean playing = false, changed = false;
	Float64 sampleInLoop = 0;
	Boolean looping		 = false;
	Float64 loopStart = 0, loopEnd = 0;
	if (this->CallHostTransportState(&playing, &changed, &sampleInLoop, &looping, &loopStart, &loopEnd) == noErr) {
		plugin->setHostIsPlaying(playing);
	}
}

template <class AUBaseT>
void MzglAUv2Unit<AUBaseT>::interleaveInput(const AudioBufferList *in, UInt32 frames) {
	interleavedIn.resize(static_cast<size_t>(frames) * 2);
	if (in == nullptr || in->mNumberBuffers == 0) {
		interleavedIn.zeros();
		return;
	}
	if (in->mNumberBuffers >= 2) {
		const float *l = static_cast<const float *>(in->mBuffers[0].mData);
		const float *r = static_cast<const float *>(in->mBuffers[1].mData);
		for (UInt32 i = 0; i < frames; ++i) {
			interleavedIn[i * 2]	 = l[i];
			interleavedIn[i * 2 + 1] = r[i];
		}
	} else if (in->mBuffers[0].mNumberChannels >= 2) {
		// already interleaved stereo
		std::memcpy(interleavedIn.data(), in->mBuffers[0].mData, sizeof(float) * frames * 2);
	} else {
		const float *m = static_cast<const float *>(in->mBuffers[0].mData);
		for (UInt32 i = 0; i < frames; ++i) {
			interleavedIn[i * 2] = interleavedIn[i * 2 + 1] = m[i];
		}
	}
}

template <class AUBaseT>
void MzglAUv2Unit<AUBaseT>::runPlugin(UInt32 frames) {
	const size_t interleavedSize = static_cast<size_t>(frames) * 2;
	const int numOutBusses		 = std::max(1, plugin->getNumOutputBusses());
	if (static_cast<int>(interleavedOuts.size()) != numOutBusses) interleavedOuts.resize(numOutBusses);
	for (auto &o: interleavedOuts) o.resize(interleavedSize);
	plugin->process(&interleavedIn, interleavedOuts.data(), 2);
}

template <class AUBaseT>
void MzglAUv2Unit<AUBaseT>::deinterleaveOutput(AudioBufferList &out, int bus, UInt32 frames) const {
	const bool have = bus >= 0 && bus < static_cast<int>(interleavedOuts.size());
	if (out.mNumberBuffers >= 2) {
		float *l = static_cast<float *>(out.mBuffers[0].mData);
		float *r = static_cast<float *>(out.mBuffers[1].mData);
		if (!have) {
			std::fill_n(l, frames, 0.f);
			std::fill_n(r, frames, 0.f);
			return;
		}
		const FloatBuffer &src = interleavedOuts[bus];
		for (UInt32 i = 0; i < frames; ++i) {
			l[i] = src[i * 2];
			r[i] = src[i * 2 + 1];
		}
	} else if (out.mNumberBuffers == 1) {
		float *m = static_cast<float *>(out.mBuffers[0].mData);
		if (out.mBuffers[0].mNumberChannels >= 2) {
			if (have) std::memcpy(m, interleavedOuts[bus].data(), sizeof(float) * frames * 2);
			else std::fill_n(m, frames * 2, 0.f);
			return;
		}
		if (!have) {
			std::fill_n(m, frames, 0.f);
			return;
		}
		const FloatBuffer &src = interleavedOuts[bus];
		for (UInt32 i = 0; i < frames; ++i) {
			m[i] = 0.5f * (src[i * 2] + src[i * 2 + 1]);
		}
	}
}

template class MzglAUv2Unit<ausdk::AUMIDIEffectBase>;
template class MzglAUv2Unit<ausdk::MusicDeviceBase>;

// ---------------------------------------------------------------------------
// MzglAUv2Effect (aufx / aumf)
// ---------------------------------------------------------------------------
MzglAUv2Effect::MzglAUv2Effect(AudioComponentInstance ci, std::shared_ptr<::Plugin> plugin, const AUv2Config &config)
	: MzglAUv2Unit<ausdk::AUMIDIEffectBase>(std::move(plugin), config, ci, false) {
}

UInt32 MzglAUv2Effect::SupportedNumChannels(const AUChannelInfo **outInfo) {
	static const AUChannelInfo info[] = {{2, 2}};
	if (outInfo != nullptr) *outInfo = info;
	return 1;
}

OSStatus MzglAUv2Effect::ProcessBufferLists(AudioUnitRenderActionFlags &ioActionFlags,
											const AudioBufferList &inBuffer,
											AudioBufferList &outBuffer,
											UInt32 inFramesToProcess) AUSDK_RTSAFE {
	beginRender();
	interleaveInput(&inBuffer, inFramesToProcess);
	runPlugin(inFramesToProcess);
	deinterleaveOutput(outBuffer, 0, inFramesToProcess);
	return noErr;
}

// ---------------------------------------------------------------------------
// MzglAUv2Instrument (aumu)
// ---------------------------------------------------------------------------
UInt32 MzglAUv2Instrument::numOutputBusses(const std::shared_ptr<::Plugin> &plugin) {
	return static_cast<UInt32>(std::max(1, plugin->getNumOutputBusses()));
}

MzglAUv2Instrument::MzglAUv2Instrument(AudioComponentInstance ci,
									   std::shared_ptr<::Plugin> plugin,
									   const AUv2Config &config)
	: MzglAUv2Unit<ausdk::MusicDeviceBase>(plugin, config, ci, 0u, numOutputBusses(plugin)) {
}

UInt32 MzglAUv2Instrument::SupportedNumChannels(const AUChannelInfo **outInfo) {
	static const AUChannelInfo info[] = {{0, 2}};
	if (outInfo != nullptr) *outInfo = info;
	return 1;
}

bool MzglAUv2Instrument::StreamFormatWritable(AudioUnitScope scope, AudioUnitElement element) {
	return !IsInitialized();
}

OSStatus MzglAUv2Instrument::Render(AudioUnitRenderActionFlags &ioActionFlags,
									const AudioTimeStamp &inTimeStamp,
									UInt32 inNumberFrames) {
	beginRender();
	interleaveInput(nullptr, inNumberFrames);
	runPlugin(inNumberFrames);

	// One Render call fills every output bus. With a single bus AUBase has already
	// prepared (or pointed us at the host's) buffer list; with several, AUBase
	// always renders into its own per-element buffers and copies each bus to the
	// host as it is pulled (see AUBase::DoRenderBus), so prepare them all here.
	const UInt32 numBusses = Outputs().GetNumberOfElements();
	for (UInt32 b = 0; b < numBusses; ++b) {
		AudioBufferList &abl = numBusses > 1 ? Output(b).PrepareBuffer(inNumberFrames) : Output(b).GetBufferList();
		deinterleaveOutput(abl, static_cast<int>(b), inNumberFrames);
	}
	return noErr;
}

} // namespace mzglau

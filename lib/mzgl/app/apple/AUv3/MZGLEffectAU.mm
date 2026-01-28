#import "MZGLEffectAU.h"

#import <AVFoundation/AVFoundation.h>
#import <CoreAudioKit/CoreAudioKit.h>
#include <os/log.h>
#include <mach/mach_time.h>
#include <thread>

#import "BufferedAudioBus.hpp"
#include "Plugin.h"
#include "log.h"
#include "FloatBuffer.h"
#include "Midi.h"

#if !TARGET_OS_IOS
#	ifndef USING_DESKTOP_AUV3
#		define USING_DESKTOP_AUV3
#	endif
#endif

#define AULog(fmt, ...) NSLog(@"[MZGLEffectAU %d] %@", inst, [NSString stringWithFormat:(fmt), ##__VA_ARGS__]);

static int instanceNumber = 0;

@interface MZGLEffectAU () {
	int inst;
}

@property(nonatomic, readwrite) AUParameterTree *parameterTree;
@property AUAudioUnitBusArray *inputBusArray;
@property AUAudioUnitBusArray *outputBusArray;
@end

static AUAudioUnitPreset *NewAUPreset(NSInteger number, NSString *name) {
	AUAudioUnitPreset *aPreset = [AUAudioUnitPreset new];
	aPreset.number			   = number;
	aPreset.name			   = name;
	return aPreset;
}

struct Blocks {
	__block AUHostMusicalContextBlock musicalContext;
	__block AUHostTransportStateBlock transportState;
	__block AUMIDIOutputEventBlock midiOut;
};

@implementation MZGLEffectAU {
	std::shared_ptr<Plugin> plugin;
	Blocks blocks;
	BufferedInputBus _inputBus;

	std::vector<FloatBuffer> outputBuffers;
	FloatBuffer inputBus;

	AUAudioUnitPreset *_currentPreset;
	NSInteger _currentFactoryPresetIndex;
	NSArray<AUAudioUnitPreset *> *_factoryPresets;
	AudioStreamBasicDescription asbd;

	bool isInstrument;
	bool allocated;
}

@synthesize parameterTree  = _parameterTree;
@synthesize factoryPresets = _factoryPresets;

- (std::shared_ptr<Plugin>)getPlugin {
	return plugin;
}

- (NSIndexSet *)supportedViewConfigurations:
	(NSArray<AUAudioUnitViewConfiguration *> *)availableViewConfigurations {
	NSMutableIndexSet *configs = [[NSMutableIndexSet alloc] init];
	for (int i = 0; i < [availableViewConfigurations count]; i++) {
		[configs addIndex:i];
	}
	return configs;
}

- (instancetype)initWithPlugin:(std::shared_ptr<Plugin>)_plugin
	   andComponentDescription:(AudioComponentDescription)componentDescription
					   options:(AudioComponentInstantiationOptions)options
						 error:(NSError **)outError {
	self = [super initWithComponentDescription:componentDescription options:options error:outError];

	if (self == nil) {
		return nil;
	}

	[self setupWithPlugin:_plugin andComponentDescription:componentDescription];
	return self;
}

- (instancetype)initWithPlugin:(std::shared_ptr<Plugin>)_plugin
	   andComponentDescription:(AudioComponentDescription)componentDescription
						 error:(NSError **)outError {
	self = [super initWithComponentDescription:componentDescription error:outError];

	if (self == nil) {
		return nil;
	}

	[self setupWithPlugin:_plugin andComponentDescription:componentDescription];
	return self;
}

- (void)setupMidiOutput {
#ifdef PLUGIN_CAN_SEND_MIDI_TO_HOST
	__weak __typeof__(self) weakSelf  = self;
	plugin->onSendMidiToAudioUnitHost = [weakSelf](const MidiMessage &midiMessage,
												   std::optional<uint64_t> timestampInNanoSeconds) {
		static constexpr uint8_t cable			   = 0;
		static constexpr AUEventSampleTime sendNow = 0;
		if (midiMessage.getBytes().empty()) {
			return;
		}

		if (!midiMessage.isSysex()) {
			auto data						 = midiMessage.getBytes();
			__strong __typeof__(self) strong = weakSelf;
			if (strong->blocks.midiOut) {
				AUEventSampleTime timestamp = sendNow;
				if (timestampInNanoSeconds.has_value()) {
					timestamp = static_cast<AUEventSampleTime>(
						(static_cast<double>(*timestampInNanoSeconds) / 1.0e9) * strong->plugin->getSampleRate());
				}

				strong->blocks.midiOut(timestamp, cable, data.size(), data.data());
			}
		}
	};
#endif
}

- (void)setupIsRunningCallback {
	__weak __typeof__(self) weakSelf = self;
	plugin->isRunning				 = [weakSelf, plug = plugin.get()]() -> bool {
		   if ([weakSelf respondsToSelector:@selector(isRunning)]) {
			   if (@available(iOS 11.0, *)) {
				   return [weakSelf isRunning];
			   }
			   NSLog(@"Error: audiounit.isRunning doesn't exist on this system");
			   return plug->hasStarted;
		   } else {
			   NSLog(@"Error: audiounit.isRunning doesn't exist on this system");
			   return plug->hasStarted;
		   }
	};
}

- (void)setupFactoryPresets {
	_currentFactoryPresetIndex = 0;
	NSMutableArray *p		   = [[NSMutableArray alloc] init];
	if (auto presetManager = plugin->getPresetManager()) {
		auto presetNames = presetManager->getFactoryPresetNames();
		for (int i = 0; i < presetNames.size(); i++) {
			[p addObject:NewAUPreset(i, [NSString stringWithUTF8String:presetNames[i].c_str()])];
		}
	}

	_factoryPresets = [p copy];

	__weak __typeof__(self) weakSelf					   = self;
	plugin->getPresetManager()->getUserPresetNamesCallback = [weakSelf]() -> std::vector<std::string> {
		std::vector<std::string> names;
		for (NSUInteger i = 0; i < [[weakSelf userPresets] count]; i++) {
			names.push_back([[[[weakSelf userPresets] objectAtIndex:i] name] UTF8String]);
		}
		return names;
	};

	_currentPreset = _factoryPresets.firstObject;
}

- (void)setupParameters {
#if TARGET_OS_IOS
	NSMutableArray *paramList = [[NSMutableArray alloc] init];

	const auto numParams = plugin->getNumParams();
	for (int paramIndex = 0; paramIndex < numParams; ++paramIndex) {
		auto p = plugin->getParam(paramIndex);
		if (p == nullptr) {
			continue;
		}
		AudioUnitParameterID paramAddr = paramIndex;
		NSString *paramName			   = [NSString stringWithUTF8String:p->name.c_str()];

		AUParameter *param = [AUParameterTree
			createParameterWithIdentifier:paramName
									 name:paramName
								  address:paramAddr
									  min:p->from
									  max:p->to
									 unit:kAudioUnitParameterUnit_Generic
								 unitName:nil
									flags:kAudioUnitParameterFlag_IsReadable | kAudioUnitParameterFlag_IsWritable
										  | kAudioUnitParameterFlag_CanRamp
							 valueStrings:nil
					  dependentParameters:nil];
		param.value		   = p->get();
		[paramList addObject:param];
	}

	_parameterTree = [AUParameterTree createTreeWithChildren:paramList];

	__weak __typeof__(self) weakSelf   = self;
	plugin->sendUpdatedParameterToHost = [weakSelf](unsigned int i, float f) {
		[[[weakSelf.parameterTree allParameters] objectAtIndex:i] setValue:f];
	};

	_parameterTree.implementorValueObserver = ^(AUParameter *param, AUValue value) {
	  __strong __typeof__(self) strongSelf = weakSelf;
	  if (!strongSelf) return;

	  if (param.address >= 0 && param.address < strongSelf->plugin->getNumParams()) {
		  strongSelf->plugin->hostUpdatedParameter(static_cast<unsigned int>(param.address), value);
	  }
	};

	_parameterTree.implementorValueProvider = ^AUValue(AUParameter *param) {
	  __strong __typeof__(self) strongSelf = weakSelf;
	  if (strongSelf && param.address >= 0 && param.address < strongSelf->plugin->getNumParams()) {
		  return strongSelf->plugin->getParam(static_cast<unsigned int>(param.address))->get();
	  }
	  return 0.f;
	};

	_parameterTree.implementorStringFromValueCallback = ^(AUParameter *param, const AUValue *__nullable valuePtr) {
	  AUValue value						   = valuePtr == nil ? param.value : *valuePtr;
	  __strong __typeof__(self) strongSelf = weakSelf;
	  if (strongSelf && param.address >= 0 && param.address < strongSelf->plugin->getNumParams()) {
		  if (strongSelf->plugin->getParam(static_cast<unsigned int>(param.address))->type
			  == PluginParameter::Type::Int) {
			  return [NSString stringWithFormat:@"%.0f", value];
		  }
		  return [NSString stringWithFormat:@"%.2f", value];
	  }
	  return @"?";
	};
#else
	NSMutableArray *paramList = [[NSMutableArray alloc] init];

	const auto numParams = plugin->getNumParams();
	for (int paramIndex = 0; paramIndex < numParams; ++paramIndex) {
		auto p = plugin->getParam(paramIndex);
		if (!p) continue;

		NSString *paramName = [NSString stringWithUTF8String:p->name.c_str()];
		AUParameter *param	= [AUParameterTree
			 createParameterWithIdentifier:paramName
									  name:paramName
								   address:paramIndex
									   min:p->from
									   max:p->to
									  unit:kAudioUnitParameterUnit_Generic
								  unitName:nil
									 flags:kAudioUnitParameterFlag_IsReadable | kAudioUnitParameterFlag_IsWritable
										   | kAudioUnitParameterFlag_CanRamp
							  valueStrings:nil
					   dependentParameters:nil];

		param.value = p->get();
		[paramList addObject:param];
	}

	_parameterTree = [AUParameterTree createTreeWithChildren:paramList];

	MZGLEffectAU *selfPtr			   = self;
	std::shared_ptr<Plugin> pluginCopy = plugin;

	plugin->sendUpdatedParameterToHost = [selfPtr](unsigned int i, float f) {
		if (i < [[selfPtr->_parameterTree allParameters] count]) {
			[selfPtr->_parameterTree allParameters][i].value = f;
		}
	};

	_parameterTree.implementorValueObserver = ^(AUParameter *param, AUValue value) {
	  if (pluginCopy && param.address >= 0 && param.address < pluginCopy->getNumParams()) {
		  pluginCopy->hostUpdatedParameter(static_cast<unsigned int>(param.address), value);
	  }
	};

	_parameterTree.implementorValueProvider = ^AUValue(AUParameter *param) {
	  if (pluginCopy && param.address >= 0 && param.address < pluginCopy->getNumParams()) {
		  return pluginCopy->getParam(static_cast<unsigned int>(param.address))->get();
	  }
	  return 0.f;
	};

	_parameterTree.implementorStringFromValueCallback = ^(AUParameter *param, const AUValue *__nullable valuePtr) {
	  AUValue value = valuePtr ? *valuePtr : param.value;
	  if (pluginCopy && param.address >= 0 && param.address < pluginCopy->getNumParams()) {
		  auto paramObj = pluginCopy->getParam(static_cast<unsigned int>(param.address));
		  if (paramObj->type == PluginParameter::Type::Int) {
			  return [NSString stringWithFormat:@"%.0f", value];
		  }
		  return [NSString stringWithFormat:@"%.2f", value];
	  }
	  return @"?";
	};
#endif
}

- (void)reserveBusses {
	static constexpr auto busSize = 8192;

	inputBus.reserve(busSize);
	outputBuffers.resize(plugin->getNumOutputBusses());

	for (auto &bus: outputBuffers) {
		bus.reserve(busSize);
	}
}

+ (AVAudioFormat *)getDefaultBusFormat {
	return [[AVAudioFormat alloc] initStandardFormatWithSampleRate:44100.0 channels:2];
}

- (void)setupStreamDescription {
	AVAudioFormat *defaultFormat = [MZGLEffectAU getDefaultBusFormat];
	asbd						 = *defaultFormat.streamDescription;
}

- (void)setupBusses {
	AVAudioFormat *defaultFormat = [MZGLEffectAU getDefaultBusFormat];

	static constexpr auto supportedAudioChannels = 8;

	_inputBus.init(defaultFormat, supportedAudioChannels);
	_inputBus.bus.name = @"Input";

	NSMutableArray *outBusses = [[NSMutableArray alloc] init];
	int numOutBusses		  = plugin->getNumOutputBusses();
	for (int i = 0; i < numOutBusses; i++) {
		AUAudioUnitBus *b = [[AUAudioUnitBus alloc] initWithFormat:defaultFormat error:nil];
		b.name			  = [NSString stringWithFormat:@"Output %d", (i + 1)];
		[outBusses addObject:b];
	}

	_outputBusArray = [[AUAudioUnitBusArray alloc] initWithAudioUnit:self
															 busType:AUAudioUnitBusTypeOutput
															  busses:outBusses];

	_inputBusArray = [[AUAudioUnitBusArray alloc] initWithAudioUnit:self
															busType:AUAudioUnitBusTypeInput
															 busses:@[ _inputBus.bus ]];

	[_outputBusArray addObserverToAllBusses:self
								 forKeyPath:@"format"
									options:0
									context:(__bridge void *_Nullable) (self)];
}

- (void)setupWithPlugin:(std::shared_ptr<Plugin>)_plugin
	andComponentDescription:(AudioComponentDescription)componentDescription {
	inst		 = instanceNumber++;
	allocated	 = false;
	plugin		 = _plugin;
	isInstrument = !(componentDescription.componentType == 'aufx' || componentDescription.componentType == 'aumf');
	self.maximumFramesToRender = 1024;

	[self setupStreamDescription];
	[self setupMidiOutput];
	[self setupIsRunningCallback];
	[self reserveBusses];
	[self setupFactoryPresets];
	[self setupParameters];
	[self setupBusses];
}

- (void)observeValueForKeyPath:(NSString *)keyPath
					  ofObject:(id)object
						change:(NSDictionary *)change
					   context:(void *)context {
	if (context == (__bridge void *_Nullable) (self)) {
		if ([keyPath isEqualToString:@"format"] && [object isKindOfClass:[AUAudioUnitBus class]]) {
			AUAudioUnitBus *bus = object;
			plugin->setSampleRate(bus.format.sampleRate);
		}

	} else {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
	}
}

- (void)dealloc {
	_factoryPresets = nil;
	plugin.reset();
}

- (NSDictionary<NSString *, id> *)fullState {
	NSMutableDictionary<NSString *, id> *state = [[super fullState] mutableCopy];

	if (!plugin->wantsToSerializeWithNSDictionary()) {
		std::vector<uint8_t> serialized;
		plugin->serialize(serialized);
		NSData *data = [NSData dataWithBytes:serialized.data() length:serialized.size()];
		[state setValue:data forKey:@"data"];
	} else {
		plugin->serializeByNSDictionary((__bridge const void *) state);
	}

	return state;
}

- (void)setFullState:(NSDictionary<NSString *, id> *)state {
	if (!plugin->wantsToSerializeWithNSDictionary()) {
		NSData *data = [state objectForKey:@"data"];
		if (data != nil) {
			auto length = [data length];
			std::vector<uint8_t> serialized;
			const uint8_t *d = (const uint8_t *) [data bytes];
			if (d != nullptr) {
				serialized.insert(serialized.end(), d, d + length);
				plugin->deserialize(serialized);
			} else {
				Log::e() << "AudioUnit setFullState, data is null";
			}
		} else {
			Log::e() << "AudioUnit setFullState, NSData is null";
		}
	} else {
		plugin->deserializeByNSDictionary((__bridge const void *) state);
	}
}

- (BOOL)supportsUserPresets {
	return YES;
}

- (BOOL)saveUserPreset:(AUAudioUnitPreset *)userPreset error:(NSError *__autoreleasing _Nullable *)outError {
	BOOL ok = [super saveUserPreset:userPreset error:outError];
	if (ok) {
		[self willChangeValueForKey:@"currentPreset"];
		super.currentPreset = userPreset;
		[self didChangeValueForKey:@"currentPreset"];
	}
	return ok;
}

- (AUAudioUnitPreset *)currentPreset {
	if (_currentPreset.number >= 0) {
		if (_currentFactoryPresetIndex >= 0 && [_factoryPresets count] > _currentFactoryPresetIndex) {
			AULog(@"Returning Current Factory Preset: %ld\n", (long) _currentFactoryPresetIndex);
			return [_factoryPresets objectAtIndex:_currentFactoryPresetIndex];
		}

		Log::e() << "AudioUnit current preset index " << _currentFactoryPresetIndex << " is out of range for "
				 << [_factoryPresets count];
		return nil;
	}
	return _currentPreset;
}

- (void)setCurrentPreset:(AUAudioUnitPreset *)currentPreset {
	if (nil == currentPreset) {
		return;
	}

	if (currentPreset.number >= 0) {
		for (AUAudioUnitPreset *factoryPreset in _factoryPresets) {
			if (currentPreset.number == factoryPreset.number) {
				plugin->getPresetManager()->loadFactoryPreset(static_cast<int>(currentPreset.number));
				_currentPreset			   = currentPreset;
				_currentFactoryPresetIndex = factoryPreset.number;
				break;
			}
		}
	} else if (nil != currentPreset.name) {
		_currentPreset = currentPreset;
		NSError *err   = nil;
		id state	   = [self presetStateFor:currentPreset error:&err];
		if (err) {
			Log::e() << "AudioUnit set current preset got error " << [[err localizedDescription] UTF8String];
			return;
		}
		if (state != nil) {
			[self setFullState:state];
		}
	}
}

- (NSArray<NSString *> *)MIDIOutputNames {
	NSMutableArray *names = [[NSMutableArray alloc] init];
	for (int i = 0; i < plugin->getNumMidiOuts(); i++) {
		[names addObject:[NSString stringWithUTF8String:plugin->getMidiOutName(i).c_str()]];
	}
	return names;
}

- (AUAudioUnitBusArray *)inputBusses {
	return _inputBusArray;
}

- (AUAudioUnitBusArray *)outputBusses {
	return _outputBusArray;
}

- (BOOL)allocateRenderResourcesAndReturnError:(NSError **)outError {
	if (![super allocateRenderResourcesAndReturnError:outError]) {
		return NO;
	}

	allocated = false;

	AUAudioUnitBus *firstOutputBus = [_outputBusArray objectAtIndexedSubscript:0];

	if (firstOutputBus.format.channelCount != _inputBus.bus.format.channelCount) {
		if (outError) {
			*outError = [NSError errorWithDomain:NSOSStatusErrorDomain
											code:kAudioUnitErr_FailedInitialization
										userInfo:nil];
		}
		self.renderResourcesAllocated = NO;
		return NO;
	}

	_inputBus.allocateRenderResources(self.maximumFramesToRender);

	if (self.musicalContextBlock) {
		blocks.musicalContext = self.musicalContextBlock;
	} else {
		blocks.musicalContext = nil;
	}
	if (self.transportStateBlock) {
		blocks.transportState = self.transportStateBlock;
	} else {
		blocks.transportState = nil;
	}

	if (@available(macOS 10.13, iOS 11.0, *)) {
		if (self.MIDIOutputEventBlock) {
			blocks.midiOut = self.MIDIOutputEventBlock;
		}
	}

	plugin->setSampleRate(firstOutputBus.format.sampleRate);
	plugin->init(asbd.mChannelsPerFrame, asbd.mChannelsPerFrame);
	allocated = true;
	return YES;
}

- (void)deallocateRenderResources {
	blocks.musicalContext = nil;
	blocks.transportState = nil;
	_inputBus.deallocateRenderResources();
	[super deallocateRenderResources];
}

- (BOOL)canProcessInPlace {
#ifdef USING_DESKTOP_AUV3
	return NO;
#else
	return YES;
#endif
}

- (AUInternalRenderBlock)internalRenderBlock {
	__block BufferedInputBus *input = &_inputBus;

	Plugin *eff	 = plugin.get();
	Blocks *blks = &blocks;

	FloatBuffer &inputBusData = inputBus;

	double sampleRate = 48000;
	if ([[self outputBusses] count] > 0) {
		AUAudioUnitBusArray *outs = [self outputBusses];
		sampleRate				  = outs[0].format.sampleRate;
		plugin->setSampleRate(sampleRate);
	}

	std::vector<FloatBuffer> &outs = outputBuffers;

	bool _isInstrument = isInstrument;

	return ^AUAudioUnitStatus(AudioUnitRenderActionFlags *actionFlags,
							  const AudioTimeStamp *timestamp,
							  AVAudioFrameCount frameCount,
							  NSInteger outputBusNumber,
							  AudioBufferList *outAudioBufferList,
							  const AURenderEvent *realtimeEventListHead,
							  AURenderPullInputBlock pullInputBlock) {
	  eff->hasStarted = true;

	  AudioBufferList *inAudioBufferList = nullptr;

	  if (outputBusNumber == 0) {
		  if (blks->transportState) {
			  AUHostTransportStateFlags transportStateFlags = 0;
			  if (blks->transportState(&transportStateFlags, nullptr, nullptr, nullptr)) {
				  bool isPlaying = transportStateFlags & AUHostTransportStateMoving;
				  if (eff->getHostIsPlaying() != isPlaying) {
					  eff->setHostIsPlaying(isPlaying);
				  }
			  }
		  }

		  if (blks->musicalContext) {
			  double bpm		  = 0;
			  double beatPosition = 0;
			  if (blks->musicalContext(&bpm, nullptr, nullptr, &beatPosition, nullptr, nullptr)) {
				  eff->bpm			= bpm;
				  eff->beatPosition = beatPosition;
			  }
		  }

		  const AURenderEvent *ev = realtimeEventListHead;
		  while (ev) {
			  if (ev->head.eventType == AURenderEventParameter
				  || ev->head.eventType == AURenderEventParameterRamp) {
				  eff->hostUpdatedParameter((unsigned int) ev->parameter.parameterAddress, ev->parameter.value);
			  } else if (ev->head.eventType == AURenderEventMIDI || ev->head.eventType == AURenderEventMIDISysEx) {
				  Float64 delay = ev->MIDI.eventSampleTime - timestamp->mSampleTime;
				  if (delay < 0) {
					  delay = 0;
				  }
				  MidiMessage m(ev->MIDI.data, ev->MIDI.length);
				  eff->midiReceivedAtTime(m, delay);
			  }
			  ev = ev->head.next;
		  }

		  bool hasInput = false;
		  if (pullInputBlock != nullptr) {
			  AUAudioUnitStatus err = input->pullInput(actionFlags, timestamp, frameCount, 0, pullInputBlock);
			  if (err == noErr) {
				  inAudioBufferList = input->mutableAudioBufferList;
				  hasInput			= true;
			  }
		  }

		  if (hasInput && inAudioBufferList != nullptr) {
			  if (inputBusData.size() != frameCount * inAudioBufferList->mNumberBuffers) {
				  inputBusData.resize(frameCount * inAudioBufferList->mNumberBuffers);
			  }

			  if (inAudioBufferList->mNumberBuffers == 1) {
				  memcpy(inputBusData.data(), inAudioBufferList->mBuffers[0].mData, sizeof(float) * frameCount);
			  } else if (inAudioBufferList->mNumberBuffers == 2) {
				  float *L = (float *) inAudioBufferList->mBuffers[0].mData;
				  float *R = (float *) inAudioBufferList->mBuffers[1].mData;

				  for (int i = 0; i < frameCount; i++) {
					  inputBusData[i * 2]	  = L[i];
					  inputBusData[i * 2 + 1] = R[i];
				  }
			  }

			  if (outs[0].size() != frameCount * inAudioBufferList->mNumberBuffers) {
				  int numSampsPerBuff = frameCount * inAudioBufferList->mNumberBuffers;
				  for (auto &o: outs) {
					  o.resize(numSampsPerBuff);
				  }
			  }

			  eff->process(&inputBusData, outs.data(), inAudioBufferList->mNumberBuffers);
		  } else {
			  if (inputBusData.size() != frameCount * 2) {
				  inputBusData.resize(frameCount * 2, 0);
			  }

			  if (outs[0].size() != frameCount * 2) {
				  int numSampsPerBuff = frameCount * 2;
				  for (auto &o: outs) {
					  o.resize(numSampsPerBuff);
				  }
			  }

			  eff->process(&inputBusData, outs.data(), 2);
		  }

		  if (eff->getNumMidiOuts() > 0 && blks->midiOut) {
			  for (auto &m: eff->midiOutMessages) {
				  AUEventSampleTime t = timestamp->mSampleTime + m.delay;
				  auto b			  = m.msg.getBytes();
				  blks->midiOut(t, m.outputNo, b.size(), b.data());
			  }
			  eff->midiOutMessages.clear();
		  }
	  }

#ifdef USING_DESKTOP_AUV3
	  if (outputBusNumber >= outs.size() || outs[outputBusNumber].size() < frameCount * 2) {
		  if (outAudioBufferList->mBuffers[0].mData != nullptr) {
			  memset(outAudioBufferList->mBuffers[0].mData, 0, sizeof(float) * frameCount);
			  if (outAudioBufferList->mNumberBuffers > 1) {
				  memset(outAudioBufferList->mBuffers[1].mData, 0, sizeof(float) * frameCount);
			  }
		  }
		  return noErr;
	  }

	  float *L = (float *) outAudioBufferList->mBuffers[0].mData;
	  float *R = (float *) outAudioBufferList->mBuffers[1].mData;

	  if (L != nullptr && R != nullptr) {
		  for (int i = 0; i < frameCount; i++) {
			  L[i] = outs[outputBusNumber][i * 2];
			  R[i] = outs[outputBusNumber][i * 2 + 1];
		  }
	  }
#else
	  // iOS handling - original in-place logic
	  if (outAudioBufferList->mBuffers[0].mData == nullptr) {
		  for (UInt32 i = 0; i < outAudioBufferList->mNumberBuffers; ++i) {
			  outAudioBufferList->mBuffers[i].mData = inAudioBufferList->mBuffers[i].mData;
		  }
	  }

	  // now get the buffer for the bus number
	  float *L = (float *) outAudioBufferList->mBuffers[0].mData;
	  float *R = (float *) outAudioBufferList->mBuffers[1].mData;
	  for (int i = 0; i < frameCount; i++) {
		  L[i] = outs[outputBusNumber][i * 2];
		  R[i] = outs[outputBusNumber][i * 2 + 1];
	  }
#endif

	  return noErr;
	};
}

@end

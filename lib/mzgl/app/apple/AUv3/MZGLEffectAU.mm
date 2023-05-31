//
//  auv3testauAudioUnit.m
//  auv3testau
//
//  Created by Marek Bereza on 17/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#import "MZGLEffectAU.h"

#import <AVFoundation/AVFoundation.h>
#import <CoreAudioKit/CoreAudioKit.h>

#import "BufferedAudioBus.hpp"
#include <os/log.h>
#include "Plugin.h"
#include "log.h"
#include <mach/mach_time.h>
#include "FloatBuffer.h"
#include "Midi.h"

using namespace std;
static int instanceNumber = 0;

@interface MZGLEffectAU () {
	int inst;
}
@property (nonatomic, readwrite) AUParameterTree *parameterTree;
@property AUAudioUnitBusArray *inputBusArray;
@property AUAudioUnitBusArray *outputBusArray;
//#if !TARGET_OS_IOS
//@property(readonly, copy, nonatomic) NSArray<NSNumber *> *channelCapabilities;
//#endif
@end

#define AULog(fmt,...) NSLog(@"[MZGLEffectAU %d] %@", inst, [NSString stringWithFormat:(fmt), ##__VA_ARGS__]);

//void AULog(NSString *s, va_list args) {
////	NSString *fullString = [[NSString alloc] initWithFormat:s arguments:args];
//	NSString *str = @"[MZGLEffectAU] ";
//	str = [str stringByAppendingFormat:s, args];
//	NSLog(str);
//
//}
static AUAudioUnitPreset* NewAUPreset(NSInteger number, NSString *name)
{
	AUAudioUnitPreset *aPreset = [AUAudioUnitPreset new];
	aPreset.number = number;
	aPreset.name = name;
	return aPreset;
}

struct Blocks {
	__block AUHostMusicalContextBlock musicalContext;
	__block AUHostTransportStateBlock transportState;
	__block AUMIDIOutputEventBlock 	  midiOut;
};

@implementation MZGLEffectAU {
	std::shared_ptr<Plugin> plugin;
	Blocks blocks;
	BufferedInputBus _inputBus;
	
	vector<FloatBuffer> outputBuffers;
	FloatBuffer inputBus;

	AUAudioUnitPreset   *_currentPreset;
	NSInteger           _currentFactoryPresetIndex;
	NSArray<AUAudioUnitPreset *> *_factoryPresets;
	AudioStreamBasicDescription asbd;

	bool isInstrument;

}
//#if !TARGET_OS_IOS
//@synthesize channelCapabilities = _channelCapabilities;
//#endif
-(std::shared_ptr<Plugin>) getPlugin {
	return plugin;
}


//
#if !TARGET_OS_IOS

- (NSArray <NSNumber *> *)channelCapabilities{
	NSArray *carray = @[@2,@2];
	return carray;
}
#endif
// TODO: Ensure lifecycle of Effect is same as AudioUnit
// TODO: Mono + Stereo support at least
// TODO: Check multiple instances
@synthesize parameterTree = _parameterTree;
@synthesize factoryPresets = _factoryPresets;

#include <thread>


- (NSIndexSet *)supportedViewConfigurations:(NSArray<AUAudioUnitViewConfiguration *> *)availableViewConfigurations {
	NSMutableIndexSet *configs = [[NSMutableIndexSet alloc] init];
	for(int i = 0; i < [availableViewConfigurations count]; i++) {
		[configs addIndex:i];
	}
	return configs;
}



- (instancetype)initWithComponentDescription:(AudioComponentDescription)componentDescription options:(AudioComponentInstantiationOptions)options error:(NSError **)outError {
	self = [super initWithComponentDescription:componentDescription options:options error:outError];
	
	if (self == nil) {
		return nil;
	}
	inst = instanceNumber++;
	isInstrument = !(componentDescription.componentType=='aufx' || componentDescription.componentType=='aumf');

	// @invalidname: Initialize a default format for the busses.
	AVAudioFormat *defaultFormat = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:44100.0 channels:2];
	asbd = *defaultFormat.streamDescription;

	
	plugin = instantiatePlugin();
	
//#if !TARGET_OS_IOS
//	// This will support any set of channels where the input number equals the output number
//	_channelCapabilities = @[@-1, @-1];
//#endif
//
//	TESTING COMMENTING THIS OUT, TO SEE IF IT FIXES SOMETHING!
//
	__weak __typeof__(self) weakSelf = self;
	auto *plug = plugin.get();
	plugin->isRunning = [weakSelf, plug]() -> bool {
		if([weakSelf respondsToSelector:@selector(isRunning)]) {
			return [weakSelf isRunning];
		} else {
			NSLog(@"Error: audiounit.isRunning doesn't exist on this system");
//			Log::d() << "Is the output bus enabled? " << _outputBus.isEnabled;
			return plug->hasStarted;
		}
	};
	
	inputBus.reserve(8192);
	
	// create output busses
	int numOutBusses = plugin->getNumOutputBusses();
	outputBuffers.resize(numOutBusses);
	for(auto &b : outputBuffers) {
		b.reserve(8192);
	}
	
	
	// Create factory preset array.
	_currentFactoryPresetIndex = 0;
	NSMutableArray *p = [[NSMutableArray alloc] init];
	auto presetNames = plugin->getPresetManager()->getFactoryPresetNames();
	for(int i = 0; i < presetNames.size(); i++) {
		[p addObject:NewAUPreset(i, [NSString stringWithUTF8String:presetNames[i].c_str()])];
	}
	_factoryPresets = [p copy];

	
	
	
	
	
	


//
//
//
//	AGAIN, COMMENTED OUT BECAUSE I'mTRYING TO FIND THE RETAIN CYCLE
//
//
//
//
//
//
//
//
//	__weak __typeof__(self) weakSelf = self;
   plugin->getPresetManager()->getUserPresetNamesCallback = [weakSelf]() -> vector<string> {
	   vector<string> names;
	   int numPresets = [[weakSelf userPresets] count];
	   for(int i = 0; i < numPresets; i++) {
		   names.push_back([[[[weakSelf userPresets] objectAtIndex:i] name] UTF8String]);
	   }
	   return names;
   };
   
   
	

	
	NSMutableArray *paramList = [[NSMutableArray alloc] init];
	
	for(int i = 0; i < plugin->getNumParams(); i++) {
		auto p = plugin->getParam(i);
		AudioUnitParameterID paramAddr = i;
		NSString *paramName = [NSString stringWithUTF8String:p->name.c_str()];
		
		// need to ask what kind of variable it is here:
		// maybe type is kAudioUnitParameterUnit_Boolean
		AUParameter *param = [AUParameterTree
							  createParameterWithIdentifier: paramName
							  name:paramName
							  address:paramAddr
							  min:p->from
							  max:p->to
							  unit:kAudioUnitParameterUnit_Generic
							  unitName:nil
							  flags:kAudioUnitParameterFlag_IsReadable|kAudioUnitParameterFlag_IsWritable|kAudioUnitParameterFlag_CanRamp
							  valueStrings:nil
							  dependentParameters:nil];
		param.value = p->get();
		[paramList addObject:param];
	}
	

	// Initialize the parameter values.
	
	
	// Create the parameter tree.
	_parameterTree = [AUParameterTree createTreeWithChildren:paramList];
	
	Plugin *eff = plugin.get();
	
	
	
	
//
//
//
//
//
//
//	BAAAHHHHHH Again comented this out reference cycle issue?? memory leak?
//
//
//
//	__weak __typeof__(self) weakSelf = self;
	eff->sendUpdatedParameterToHost = [weakSelf](unsigned int i, float f) {
		AUParameter *param = [[weakSelf.parameterTree allParameters] objectAtIndex:i];
//		Log::d() << "Sending " << f << " to host";// at " << getSeconds()*1000.f;
		[param setValue: f];
		// TODO: atHostTime:0 ?
		//		AUParameterAutomationEventTypeTouch // down
		//		AUParameterAutomationEventTypeValue // moved
		//		AUParameterAutomationEventTypeRelease // up
		//		[param setValue:f originator: nil atHostTime:0 eventType:(AUParameterAutomationEventType)eventType]);

		
	};
//
	

	
	
	
//
	// implementorValueObserver is called when a parameter changes value in the host
	
	_parameterTree.implementorValueObserver =
	^(AUParameter *param, AUValue value) {
		if(param.address>=0 && param.address<eff->getNumParams()) {
			eff->hostUpdatedParameter(param.address, value);
		
		}
	};
	
	// implementorValueProvider is called when the value needs to be refreshed.
	_parameterTree.implementorValueProvider =
	^(AUParameter *param) {
		if(param.address>=0 && param.address<eff->getNumParams()) {
			return eff->getParam(param.address)->get();
		}
		return 0.f;
	};
		
	// Create the input and output busses (AUAudioUnitBus).
	// @invalidname
	//_inputBus = [[AUAudioUnitBus alloc] initWithFormat:defaultFormat error:nil];
	_inputBus.init(defaultFormat, 8);
	_inputBus.bus.name = @"Input";
	
	NSMutableArray *outBusses = [[NSMutableArray alloc] init];
	for(int i = 0; i < numOutBusses; i++) {
		AUAudioUnitBus *b = [[AUAudioUnitBus alloc] initWithFormat:defaultFormat error:nil];
		b.name = [NSString stringWithFormat:@"Output %d", (i+1)];
		[outBusses addObject: b];
	}
	

	// Create the input and output bus arrays (AUAudioUnitBusArray).
	// @invalidname
	_inputBusArray  = [[AUAudioUnitBusArray alloc] initWithAudioUnit:self busType:AUAudioUnitBusTypeInput busses: @[_inputBus.bus]];
	_outputBusArray = [[AUAudioUnitBusArray alloc] initWithAudioUnit:self busType:AUAudioUnitBusTypeOutput busses: outBusses];
	
	[_outputBusArray addObserverToAllBusses:self forKeyPath:@"format" options:0 context:(__bridge void * _Nullable)(self)];

	
	
	// A function to provide string representations of parameter values.
	_parameterTree.implementorStringFromValueCallback = ^(AUParameter *param, const AUValue *__nullable valuePtr) {
		AUValue value = valuePtr == nil ? param.value : *valuePtr;
		if(param.address>=0 && param.address<eff->getNumParams()) {
			if(eff->getParam(param.address)->type==PluginParameter::Type::Int) {
				return [NSString stringWithFormat:@"%.0f", value];
			} else {
				return [NSString stringWithFormat:@"%.2f", value];
			}
		} else {
			return @"?";
		}
	};
	
	self.maximumFramesToRender = 1024;
	
	self.currentPreset = _factoryPresets.firstObject;
	
	
	return self;
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
	if (context == (__bridge void * _Nullable)(self)) {
		if(keyPath==@"format" && [object isKindOfClass:[AUAudioUnitBus class]]) {
			AUAudioUnitBus *bus = object;
			plugin->setSampleRate(bus.format.sampleRate);
		}

	} else {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
	}
}

-(void)dealloc {
	_factoryPresets = nil;
}

#pragma mark- AUAudioUnit (Optional Properties)

//
//- (NSDictionary<NSString *,id> *)presetStateFor:(AUAudioUnitPreset *)userPreset error:(NSError *__autoreleasing  _Nullable *)outError {
//	return [self fullStateForDocument];
//}
//
//- (NSDictionary<NSString *,id> *)fullStateForDocument {
//	return [self fullState];
//}
//
//- (void)setFullStateForDocument:(NSDictionary<NSString *,id> *)fullStateForDocument {
//	[self setFullState:fullStateForDocument];
//}

- (NSDictionary<NSString *,id> *)fullState {
	AULog(@"Full state called");
//	NSMutableDictionary<NSString*,id> *state = [super fullState];
//	NSMutableDictionary<NSString*,id> *state = [[NSMutableDictionary alloc] init];
	NSMutableDictionary<NSString*,id> *state = [[super fullState] mutableCopy];
	
	if(!plugin->wantsToSerializeWithNSDictionary()) { // normal data serialize
		vector<uint8_t> serialized;
		plugin->serialize(serialized);
		NSData *data = [NSData dataWithBytes:serialized.data() length:serialized.size()];
		[state setValue:data forKey:@"data"];
	} else {
		// file serialize
		plugin->serializeByNSDictionary((__bridge const void*)state);
	}

	return state;
}


- (void) setFullState:(NSDictionary<NSString *,id> *)state {
	//[super setFullState:state];
	AULog(@"setFullState called");
	Log::d() << "setFullState";
	if(!plugin->wantsToSerializeWithNSDictionary()) {
		AULog(@"plugin doesn't want nsdict serialization");
		NSData *data = [state objectForKey:@"data"];
		if(data!=nil) {
			AULog(@"data not null");

			uint32_t length = [data length];
			vector<uint8_t> serialized;
			const uint8_t *d = (const uint8_t*)[data bytes];
			if(d!=nullptr) {
				AULog(@"bytes not null");

				serialized.insert(serialized.end(), d, d + length);
				plugin->deserialize(serialized);
			} else {
				AULog(@"bytes null");
			}
		} else {
			AULog(@"data null");
		}
	} else {
		AULog(@"Calling nsdictionary deserialization");
		plugin->deserializeByNSDictionary((__bridge const void*)state);
	}
}

- (BOOL) supportsUserPresets {
	return YES;
}

// from Jonatan Liljedahl
- (BOOL)saveUserPreset:(AUAudioUnitPreset *)userPreset error:(NSError *__autoreleasing  _Nullable *)outError {
	BOOL ok = [super saveUserPreset:userPreset error:outError];
	if(ok) {
		[self willChangeValueForKey:@"currentPreset"];
		super.currentPreset = userPreset; // why doesn't the base class already do this?
		[self didChangeValueForKey:@"currentPreset"];
	}
	return ok;
}
//
//// THIS LOOKS A BIT IFFY TO ME, NOT SURE IF IT REALLY WORKS...
- (AUAudioUnitPreset *)currentPreset {
//	return nil;
	AULog(@"currentPreset called");
	if (_currentPreset.number >= 0) {
		if(_currentFactoryPresetIndex>=0 && [_factoryPresets count] > _currentFactoryPresetIndex) {
			AULog(@"Returning Current Factory Preset: %ld\n", (long)_currentFactoryPresetIndex);
			return [_factoryPresets objectAtIndex:_currentFactoryPresetIndex];
		} else {
			AULog(@"Preset index out of range!");
			return nil;
		}
	} else {
//		AULog(@"currentPreset returning nil because its a custom preset");
//		return nil;
		AULog(@"Returning Current Custom Preset: %ld, %@\n", (long)_currentPreset.number, _currentPreset.name);
		return _currentPreset;
	}
	
}
//
- (void)setCurrentPreset:(AUAudioUnitPreset *)currentPreset {
	if(currentPreset==nil) {
		AULog(@"setCurrentPreset called with nil");
	} else {
		AULog(@"setCurrentPreset called with num %d ('%@')", currentPreset.number, currentPreset.name);
	}
	if (nil == currentPreset) { /*AULog(@"nil passed to setCurrentPreset!");*/ return; }
//	AULog(@"preset number is %d", currentPreset.number);
	
	if (currentPreset.number >= 0) {
		// factory preset
		for (AUAudioUnitPreset *factoryPreset in _factoryPresets) {
			if (currentPreset.number == factoryPreset.number) {
				plugin->getPresetManager()->loadFactoryPreset(currentPreset.number);
				// set factory preset as current
				_currentPreset = currentPreset;
				_currentFactoryPresetIndex = factoryPreset.number;
				AULog(@"currentPreset Factory: %ld, %@\n", (long)_currentFactoryPresetIndex, factoryPreset.name);

				break;
			}
		}
	} else if (nil != currentPreset.name) {
		// set custom preset as current
		_currentPreset = currentPreset;
		NSError *err = nil;
		id state = [self presetStateFor:currentPreset error:&err];
		if(err) {
			AULog(@"Got error: %@", err);
			return;
		}
		if(state!=nil) {
			[self setFullState:state];
		}
		AULog(@"currentPreset Custom: %ld, %@\n", (long)_currentPreset.number, _currentPreset.name);
	} else {
		AULog(@"setCurrentPreset not set! - invalid AUAudioUnitPreset\n");
	}
}

#pragma mark - MIDI out
- (NSArray<NSString*>*)MIDIOutputNames {
	NSMutableArray *names = [[NSMutableArray alloc] init];
	for(int i = 0; i < plugin->getNumMidiOuts(); i++) {
		[names addObject:[NSString stringWithUTF8String: plugin->getMidiOutName(i).c_str()]];
	}
	return names;
}


//std::function<void(int, const MidiMessage &m)> sendMidiMessage = [](int,const MidiMessage &m) {};



#pragma mark - AUAudioUnit Overrides

// If an audio unit has input, an audio unit's audio input connection points.
// Subclassers must override this property getter and should return the same object every time.
// See sample code.
- (AUAudioUnitBusArray *)inputBusses {
	if(isInstrument) {
		return [super inputBusses];
	} else {
		return _inputBusArray;
	}
}

// An audio unit's audio output connection points.
// Subclassers must override this property getter and should return the same object every time.
// See sample code.
- (AUAudioUnitBusArray *)outputBusses {
	return _outputBusArray;
}

// Allocate resources required to render.
// Subclassers should call the superclass implementation.
- (BOOL)allocateRenderResourcesAndReturnError:(NSError **)outError {
	if (![super allocateRenderResourcesAndReturnError:outError]) {
		return NO;
	}
	
	AUAudioUnitBus *firstOutputBus = [_outputBusArray objectAtIndexedSubscript: 0];
	
	
	if (firstOutputBus.format.channelCount != _inputBus.bus.format.channelCount) {
		if (outError) {
			*outError = [NSError errorWithDomain:NSOSStatusErrorDomain code:kAudioUnitErr_FailedInitialization userInfo:nil];
		}
		// Notify superclass that initialization was not successful
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
		if(self.MIDIOutputEventBlock) {
			blocks.midiOut = self.MIDIOutputEventBlock;
		}
	}

	
	plugin->setSampleRate(firstOutputBus.format.sampleRate);
	plugin->init(asbd.mChannelsPerFrame, asbd.mChannelsPerFrame);
	return YES;
}

// Deallocate resources allocated in allocateRenderResourcesAndReturnError:
// Subclassers should call the superclass implementation.
- (void)deallocateRenderResources {
	// Deallocate your resources.
	blocks.musicalContext = nil;
	blocks.transportState = nil;
	_inputBus.deallocateRenderResources();
	
	
	[super deallocateRenderResources];
	
	
}

// Expresses whether an audio unit can process in place.
// In-place processing is the ability for an audio unit to transform an input signal to an
// output signal in-place in the input buffer, without requiring a separate output buffer.
// A host can express its desire to process in place by using null mData pointers in the output
// buffer list. The audio unit may process in-place in the input buffers.
// See the discussion of renderBlock.
// Partially bridged to the v2 property kAudioUnitProperty_InPlaceProcessing, the v3 property is not settable.
- (BOOL)canProcessInPlace {
	return YES;
}

#pragma mark - AUAudioUnit (AUAudioUnitImplementation)

// Block which subclassers must provide to implement rendering.
- (AUInternalRenderBlock)internalRenderBlock {
	
	
	__block BufferedInputBus *input = &_inputBus;
	
	Plugin *eff = plugin.get();
	Blocks *blks = &blocks;
	
	FloatBuffer &inputBusData = inputBus;

	// we have a chance here to get the starting sample rate.
	double sampleRate = 48000;
	if([[self outputBusses] count]>0) {
		AUAudioUnitBusArray *outs = [self outputBusses];
		sampleRate = outs[0].format.sampleRate;
		plugin->setSampleRate(sampleRate);
	}
	
	
	// now run the plugin
	vector<FloatBuffer> &outs = outputBuffers;
	
	bool _isInstrument = isInstrument;

	return ^AUAudioUnitStatus(AudioUnitRenderActionFlags *actionFlags, const AudioTimeStamp *timestamp, AVAudioFrameCount frameCount, NSInteger outputBusNumber, AudioBufferList *outAudioBufferList, const AURenderEvent *realtimeEventListHead, AURenderPullInputBlock pullInputBlock) {

		eff->hasStarted = true;
		
		AudioBufferList *inAudioBufferList = nullptr;
		
		// only do all this on the first render block
		if(outputBusNumber==0) {
			if(blks->transportState) {
				AUHostTransportStateFlags transportStateFlags = 0;
				if(blks->transportState(&transportStateFlags, nullptr, nullptr, nullptr)) {
					bool isPlaying = transportStateFlags & AUHostTransportStateMoving;
					if(eff->getHostIsPlaying()!=isPlaying) {
						eff->setHostIsPlaying(isPlaying);
					}
				}
			}
			
			if(blks->musicalContext) {
			   double bpm = 0;
			   double beatPosition = 0;
			   if(blks->musicalContext( &bpm, nullptr, nullptr, &beatPosition, nullptr, nullptr)) {
				   eff->bpm = bpm;
				   eff->beatPosition = beatPosition;
			   }
			}
			
			const AURenderEvent *ev = realtimeEventListHead;
			while (ev) {
			  if(ev->head.eventType == AURenderEventParameter
			  || ev->head.eventType == AURenderEventParameterRamp) {
				  
				  eff->hostUpdatedParameter((unsigned int)ev->parameter.parameterAddress, ev->parameter.value);
			  } else if(ev->head.eventType == AURenderEventMIDI
						|| ev->head.eventType == AURenderEventMIDISysEx) {
				  Float64 delay = ev->MIDI.eventSampleTime - timestamp->mSampleTime;
				  if(delay<0) {
					  delay = 0;
				  }
				  MidiMessage m(ev->MIDI.data, ev->MIDI.length);
				  eff->midiReceivedAtTime(m, delay);
			  }
			  ev = ev->head.next;
			}

			if(outputBusNumber!=0) return noErr;
			
			// pull in samples to filter
			if(!_isInstrument) {
				AUAudioUnitStatus err = input->pullInput(actionFlags, timestamp, frameCount, 0, pullInputBlock);
				if(err != 0) {
					return err;
				}
				
				inAudioBufferList = input->mutableAudioBufferList;

				if(inputBusData.size() != frameCount * inAudioBufferList->mNumberBuffers) {
					inputBusData.resize(frameCount * inAudioBufferList->mNumberBuffers);
				}
				
				// do processing here
				// interleave audio channels
				if(inAudioBufferList->mNumberBuffers==1) {
					// mono
					memcpy(inputBusData.data(), inAudioBufferList->mBuffers[0].mData, sizeof(float) * frameCount);
				} else if(inAudioBufferList->mNumberBuffers==2) {
					// stereo
					float *L = (float*)inAudioBufferList->mBuffers[0].mData;
					float *R = (float*)inAudioBufferList->mBuffers[1].mData;
					
					for(int i = 0; i < frameCount; i++) {
						inputBusData[ i*2 ] = L[i];
						inputBusData[i*2+1] = R[i];
					}
				}
				
				if(outs[0].size()!=frameCount*inAudioBufferList->mNumberBuffers) {
					int numSampsPerBuff = frameCount*inAudioBufferList->mNumberBuffers;
					// as long as buffer size is less than 4096, memory is already allocated
					for(auto &o : outs) {
						o.resize(numSampsPerBuff);
					}
				}
				
				eff->process(&inputBusData, outs.data(), inAudioBufferList->mNumberBuffers);
			} else {
				
				if(inputBusData.size() != frameCount * outAudioBufferList->mNumberBuffers) {
					 inputBusData.resize(frameCount * outAudioBufferList->mNumberBuffers, 0);
				}
				
				if(outs[0].size()!=frameCount*outAudioBufferList->mNumberBuffers) {
					int numSampsPerBuff = frameCount*outAudioBufferList->mNumberBuffers;
					// as long as buffer size is less than 4096, memory is already allocated
					for(auto &o : outs) {
						o.resize(numSampsPerBuff);
					}
				}
				
				
				eff->process(&inputBusData, outs.data(), outAudioBufferList->mNumberBuffers);
			}
			
			// now do any midi output events
			if(eff->getNumMidiOuts()>0 && blks->midiOut) {
				for(auto &m : eff->midiOutMessages) {
//					typedef OSStatus (^AUMIDIOutputEventBlock)(AUEventSampleTime eventSampleTime, uint8_t cable, NSInteger length, const uint8_t *midiBytes);
					AUEventSampleTime t = timestamp->mSampleTime + m.delay;
					auto b = m.msg.getBytes();
					blks->midiOut(t, m.outputNo, b.size(), b.data());
				}
				eff->midiOutMessages.clear();
			}
		}
		
		
		
		// copy into the audioOutBufferList the appropriate buffer
		
		
		// now outs has all the samples
		
//         Important:
//         If the caller passed non-null output pointers (outputData->mBuffers[x].mData), use those.
//
//         If the caller passed null output buffer pointers, process in memory owned by the Audio Unit
//         and modify the (outputData->mBuffers[x].mData) pointers to point to this owned memory.
//         The Audio Unit is responsible for preserving the validity of this memory until the next call to render,
//         or deallocateRenderResources is called.
//
//         If your algorithm cannot process in-place, you will need to preallocate an output buffer
//         and use it here.
//
//         See the description of the canProcessInPlace property.
		 
		// If passed null output buffer pointers, process in-place in the input buffer.
		if (outAudioBufferList->mBuffers[0].mData == nullptr) {
			for (UInt32 i = 0; i < outAudioBufferList->mNumberBuffers; ++i) {
				outAudioBufferList->mBuffers[i].mData = inAudioBufferList->mBuffers[i].mData;
			}
		}
		
		// now get the buffer for the bus number
		float *L = (float*)outAudioBufferList->mBuffers[0].mData;
		float *R = (float*)outAudioBufferList->mBuffers[1].mData;
		for(int i = 0; i < frameCount; i++) {
			L[i] = outs[outputBusNumber][i*2];
			R[i] = outs[outputBusNumber][i*2 + 1];
		}
  
		return noErr;
	};
}

@end

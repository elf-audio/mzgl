//
//  AudioSystem.cpp
//  auv3test
//
//  Created by Marek Bereza on 21/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#include "AudioSystemIOS.h"
#include <string>
#import <AVFoundation/AVFoundation.h>
#include "log.h"
using namespace std;

// don't know why this is needed
#define MAX_BUFFER_SIZE 4096

#define kOutputBus		0
#define kInputBus		1

typedef struct {
	AudioBufferList *bufferList;
	AudioUnit remoteIO;
	AudioSystemIOS *stream;
} AudioSystemInputStreamContext;

AudioSystemInputStreamContext *audioStreamContext = NULL;

static OSStatus audioSystemOutRenderCallback(void *inRefCon,
											 AudioUnitRenderActionFlags *ioActionFlags,
											 const AudioTimeStamp *inTimeStamp,
											 UInt32 inBusNumber,
											 UInt32 inNumberFrames,
											 AudioBufferList *ioData) {
	AudioSystem *system		 = (AudioSystem *) inRefCon;
	AudioBuffer *audioBuffer = &ioData->mBuffers[0];

	// clearing the buffer before handing it off to the user
	// this saves us from horrible noises if the user chooses not to write anything
	memset(audioBuffer->mData, 0, audioBuffer->mDataByteSize);

	int bufferSize = (audioBuffer->mDataByteSize / sizeof(Float32)) / audioBuffer->mNumberChannels;
	bufferSize	   = MIN(bufferSize, MAX_BUFFER_SIZE / audioBuffer->mNumberChannels);

	if (system->outputCallback) {
		system->outputCallback((float *) audioBuffer->mData, bufferSize, audioBuffer->mNumberChannels);
	}

	return noErr;
}

static OSStatus audioSystemInRenderCallback(void *inRefCon,
											AudioUnitRenderActionFlags *ioActionFlags,
											const AudioTimeStamp *inTimeStamp,
											UInt32 inBusNumber,
											UInt32 inNumberFrames,
											AudioBufferList *ioData) {
	AudioSystemInputStreamContext *context = (AudioSystemInputStreamContext *) inRefCon;
	AudioBufferList *bufferList			   = context->bufferList;
	AudioBuffer *buffer					   = &bufferList->mBuffers[0];

	// make sure our buffer is big enough
	UInt32 necessaryBufferSize = inNumberFrames * sizeof(Float32);
	if (buffer->mDataByteSize < necessaryBufferSize) {
		free(buffer->mData);
		buffer->mDataByteSize = necessaryBufferSize;
		buffer->mData		  = malloc(necessaryBufferSize);
	}

	// we need to store the original buffer size, since AudioUnitRender seems to change the value
	// of the AudioBufferList's mDataByteSize (at least in the simulator). We need to write it back
	// later, or else we'll end up reallocating continuously in the render callback (BAD!)
	//UInt32 bufferSize = buffer->mDataByteSize;

	OSStatus status = AudioUnitRender(
		context->remoteIO, ioActionFlags, inTimeStamp, inBusNumber, inNumberFrames, context->bufferList);

	if (status != noErr) {
		@autoreleasepool {
			NSLog(@"Could not render input audio samples %d", status);
		}
		return status;
	}

	if (context->stream->inputCallback) {
		// BUG: again, not coping with more than 1 input channel
		context->stream->inputCallback((float *) bufferList->mBuffers[0].mData,
									   bufferList->mBuffers[0].mDataByteSize / sizeof(Float32),
									   bufferList->mBuffers[0].mNumberChannels);
	}
	return noErr;
}

bool AudioSystemIOS::checkStatus(OSStatus status) {
	if (status == noErr) {
		return true;
	}

	string errorCode = auErrString(status) + " (" + to_string((int) status) + ")";

	NSLog(@"ERROR: %s", errorCode.c_str());
	return false;
}

// TODO: deal with interruptions like oF does

void AudioSystemIOS::configureAudioSession() {
	NSError *err				 = nil;
	AVAudioSession *audioSession = [AVAudioSession sharedInstance];

	// need to configure set the audio category, and override to it route the audio to the speaker
	if ([audioSession respondsToSelector:@selector(setCategory:withOptions:error:)]) {
		// we're on iOS 6 or greater, so use the AVFoundation API
		NSString *category = AVAudioSessionCategoryPlayback;
		if (numInChannels > 0) {
			category = AVAudioSessionCategoryPlayAndRecord;
		}

		// options. Need MixWithOthers for AudioBus
		AVAudioSessionCategoryOptions opts =
			AVAudioSessionCategoryOptionDefaultToSpeaker | AVAudioSessionCategoryOptionMixWithOthers;

		if (![audioSession setCategory:category withOptions:opts error:&err]) {
			NSLog(@"ERROR: %@", err.localizedDescription);
			err = nil;
		}
	}

	if (![audioSession setActive:YES error:&err]) {
		NSLog(@"configureAudioSession Error: %@", [err localizedDescription]);
		// if we can't even activate the session, we better abort early
		return;
	}

	// setting sample rate (this has different selectors for iOS 5- and iOS 6+)
	double trueSampleRate = sampleRate;
	if ([audioSession respondsToSelector:@selector(setPreferredSampleRate:error:)]) {
		if (![audioSession setPreferredSampleRate:sampleRate error:&err]) {
			NSLog(@"configureAudioSession Error: %@", [err localizedDescription]);
			err = nil;
		}
		trueSampleRate = [audioSession sampleRate];
	}
	if (trueSampleRate != sampleRate) {
		sampleRate = trueSampleRate;
	}

	// setting buffer size
	NSTimeInterval bufferDuration = bufferSize / trueSampleRate;
	if (![audioSession setPreferredIOBufferDuration:bufferDuration error:&err]) {
		NSLog(@"configureAudioSession Error: %@", [err localizedDescription]);
		err = nil;
	}
}

void AudioSystemIOS::setup(int numInputChannels, int numOutputChannels) {
	this->numInChannels	 = numInputChannels;
	this->numOutChannels = numOutputChannels;
	configureAudioSession();
	configureAudioUnit();
}

static void routeChangeNotificationHandler(CFNotificationCenterRef center,
										   void *observer,
										   CFStringRef name,
										   const void *object,
										   CFDictionaryRef userInfo) {
	(static_cast<AudioSystemIOS *>(observer))->handleRouteChange();
}
void AudioSystemIOS::handleRouteChange() {
	//	Log::w() << "Got route change notification - should re-query samplerate";
	Log::d() << "Route changed to " << getInput().name;
}

AudioSystemIOS::AudioSystemIOS() {
	CFNotificationCenterAddObserver(CFNotificationCenterGetLocalCenter()

										,
									this,
									routeChangeNotificationHandler,
									(__bridge CFStringRef) AVAudioSessionRouteChangeNotification,
									NULL,
									CFNotificationSuspensionBehaviorDeliverImmediately);
}

AudioSystemIOS::~AudioSystemIOS() {
	if (isRunning()) {
		stop();
	}
	running = false;
	AudioUnitUninitialize(audioUnit);
	AudioComponentInstanceDispose(audioUnit);
	audioUnit = nil;
	if (audioStreamContext != nullptr) {
		deleteContext(audioStreamContext);
	}
	CFNotificationCenterRemoveEveryObserver(CFNotificationCenterGetLocalCenter(), this);
}

void AudioSystemIOS::start() {
	[[AVAudioSession sharedInstance] setActive:YES error:nil];

	////////////////////////////////////////////////////////////////////////////////
	// START THE BASTARD
	checkStatus(AudioOutputUnitStart(audioUnit));
	running = true;
}

void AudioSystemIOS::stop() {
	[[AVAudioSession sharedInstance] setActive:NO error:nil];
	AudioOutputUnitStop(audioUnit);
}

void updateChannelCount(int &channelsRequested, const AudioPort &port, const std::vector<AudioPort> &allPorts) {
	if (channelsRequested != std::numeric_limits<int>::max()) {
		return;
	}
	
	auto iter=  std::find_if(allPorts.begin(), allPorts.end(), [&](auto && in) {
		return in.name == port.name;
	});

	if (iter != allPorts.end()) {
		channelsRequested = iter->numInChannels;
	}
}

void AudioSystemIOS::configureAudioUnit() {
	//---------------------------------------------------------- audio unit.

	// Configure the search parameters to find the default playback output unit
	// (called the kAudioUnitSubType_RemoteIO on iOS but
	// kAudioUnitSubType_DefaultOutput on Mac OS X)
	AudioComponentDescription desc = {.componentType		 = kAudioUnitType_Output,
									  .componentSubType		 = kAudioUnitSubType_RemoteIO,
									  .componentManufacturer = kAudioUnitManufacturer_Apple};

	// get component and get audio units.
	AudioComponent inputComponent = AudioComponentFindNext(NULL, &desc);
	checkStatus(AudioComponentInstanceNew(inputComponent, &audioUnit));

	UInt32 on = 1;

	updateChannelCount(numInChannels, getInput(), getInputs());
	updateChannelCount(numOutChannels, getOutput(), getOutputs());

	////////////////////////////////////////////////////////////////////////////////
	// OUTPUT

	if (numOutChannels > 0) {
		// enable output out of AudioUnit.
		checkStatus(AudioUnitSetProperty(
			audioUnit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Output, kOutputBus, &on, sizeof(on)));

		//---------------------------------------------------------- format.

		// Describe format
		AudioStreamBasicDescription audioFormat = {.mSampleRate		  = sampleRate,
												   .mFormatID		  = kAudioFormatLinearPCM,
												   .mFormatFlags	  = kAudioFormatFlagsNativeFloatPacked,
												   .mFramesPerPacket  = 1,
												   .mChannelsPerFrame = (UInt32) numOutChannels,
												   .mBytesPerFrame	  = (UInt32) sizeof(Float32) * numOutChannels,
												   .mBytesPerPacket	  = (UInt32) sizeof(Float32) * numOutChannels,
												   .mBitsPerChannel	  = sizeof(Float32) * 8};

		// Apply format
		checkStatus(AudioUnitSetProperty(audioUnit,
										 kAudioUnitProperty_StreamFormat,
										 kAudioUnitScope_Input,
										 kOutputBus,
										 &audioFormat,
										 sizeof(AudioStreamBasicDescription)));

		AURenderCallbackStruct callback = {audioSystemOutRenderCallback, this};
		checkStatus(AudioUnitSetProperty(audioUnit,
										 kAudioUnitProperty_SetRenderCallback,
										 kAudioUnitScope_Global,
										 kOutputBus,
										 &callback,
										 sizeof(callback)));
	}

	////////////////////////////////////////////////////////////////////////////////
	// INPUT CONFIG

	if (numInChannels > 0) {
		// enable input to AudioUnit.
		checkStatus(AudioUnitSetProperty(
			audioUnit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input, kInputBus, &on, sizeof(on)));

		// enable output out of AudioUnit.
		// TODO BUG - this might be a redundant call as it will be called in the output config above, don't know if
		// you can have an audio in without an audio out though.
		checkStatus(AudioUnitSetProperty(
			audioUnit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Output, kOutputBus, &on, sizeof(on)));

		//---------------------------------------------------------- format.

		// Describe format
		AudioStreamBasicDescription audioFormat = {.mSampleRate		  = sampleRate,
												   .mFormatID		  = kAudioFormatLinearPCM,
												   .mFormatFlags	  = kAudioFormatFlagsNativeFloatPacked,
												   .mFramesPerPacket  = 1,
												   .mChannelsPerFrame = (UInt32) numInChannels,
												   .mBytesPerFrame	  = (UInt32) sizeof(Float32) * numInChannels,
												   .mBytesPerPacket	  = (UInt32) sizeof(Float32) * numInChannels,
												   .mBitsPerChannel	  = sizeof(Float32) * 8};

		// Apply format
		checkStatus(AudioUnitSetProperty(audioUnit,
										 kAudioUnitProperty_StreamFormat,
										 kAudioUnitScope_Output,
										 kInputBus,
										 &audioFormat,
										 sizeof(audioFormat)));

		//---------------------------------------------------------- callback.

		// input callback
		if (audioStreamContext != NULL) {
			deleteContext(audioStreamContext);
		}
		audioStreamContext				= new AudioSystemInputStreamContext;
		AURenderCallbackStruct callback = {audioSystemInRenderCallback, audioStreamContext};

		audioStreamContext->remoteIO = audioUnit;
		audioStreamContext->stream	 = this;

		checkStatus(AudioUnitSetProperty(audioUnit,
										 kAudioOutputUnitProperty_SetInputCallback,
										 kAudioUnitScope_Global,
										 kInputBus,
										 &callback,
										 sizeof(callback)));

		//---------------------------------------------------------- make buffers.

		UInt32 bufferListSize = offsetof(AudioBufferList, mBuffers[0]) + (sizeof(AudioBuffer) * numInChannels);
		audioStreamContext->bufferList				   = (AudioBufferList *) malloc(bufferListSize);
		audioStreamContext->bufferList->mNumberBuffers = numInChannels;

		for (int i = 0; i < audioStreamContext->bufferList->mNumberBuffers; i++) {
			audioStreamContext->bufferList->mBuffers[i].mNumberChannels = 1;
			audioStreamContext->bufferList->mBuffers[i].mDataByteSize	= bufferSize * sizeof(Float32);
			audioStreamContext->bufferList->mBuffers[i].mData			= calloc(bufferSize, sizeof(Float32));
		}
	}

	checkStatus(AudioUnitInitialize(audioUnit));
}

void AudioSystemIOS::deleteContext(void *ctx) {
	AudioSystemInputStreamContext *c = (AudioSystemInputStreamContext *) ctx;

	for (int i = 0; i < c->bufferList->mNumberBuffers; i++) {
		free(c->bufferList->mBuffers[i].mData);
	}
	free(c->bufferList);
	delete c;
}

string AudioSystemIOS::auErrString(OSStatus status) {
	if (status == kAudioUnitErr_InvalidProperty) {
		return "kAudioUnitErr_InvalidProperty";
	} else if (status == kAudioUnitErr_InvalidParameter) {
		return "kAudioUnitErr_InvalidParameter";
	} else if (status == kAudioUnitErr_InvalidElement) {
		return "kAudioUnitErr_InvalidElement";
	} else if (status == kAudioUnitErr_NoConnection) {
		return "kAudioUnitErr_NoConnection";
	} else if (status == kAudioUnitErr_FailedInitialization) {
		return "kAudioUnitErr_FailedInitialization";
	} else if (status == kAudioUnitErr_TooManyFramesToProcess) {
		return "kAudioUnitErr_TooManyFramesToProcess";
	} else if (status == kAudioUnitErr_InvalidFile) {
		return "kAudioUnitErr_InvalidFile";
	} else if (status == kAudioUnitErr_FormatNotSupported) {
		return "kAudioUnitErr_FormatNotSupported";
	} else if (status == kAudioUnitErr_Uninitialized) {
		return "kAudioUnitErr_Uninitialized";
	} else if (status == kAudioUnitErr_InvalidScope) {
		return "kAudioUnitErr_InvalidScope";
	} else if (status == kAudioUnitErr_PropertyNotWritable) {
		return "kAudioUnitErr_PropertyNotWritable";
	} else if (status == kAudioUnitErr_CannotDoInCurrentContext) {
		return "kAudioUnitErr_CannotDoInCurrentContext";
	} else if (status == kAudioUnitErr_InvalidPropertyValue) {
		return "kAudioUnitErr_InvalidPropertyValue";
	} else if (status == kAudioUnitErr_PropertyNotInUse) {
		return "kAudioUnitErr_PropertyNotInUse";
	} else if (status == kAudioUnitErr_Initialized) {
		return "kAudioUnitErr_Initialized";
	} else if (status == kAudioUnitErr_InvalidOfflineRender) {
		return "kAudioUnitErr_InvalidOfflineRender";
	} else if (status == kAudioUnitErr_Unauthorized) {
		return "kAudioUnitErr_Unauthorized";
	} else {
		return "Unknown";
	}
}

vector<AudioPort> AudioSystemIOS::getInputs() {
	vector<AudioPort> ports;

	NSArray<AVAudioSessionDataSourceDescription *> *descs = [[AVAudioSession sharedInstance] outputDataSources];
	Log::e() << "---Got " << [descs count] << " data sources";
	for (AVAudioSessionDataSourceDescription *desc in descs) {
		AudioPort p;
		p.name = [desc.dataSourceName UTF8String];
		ports.push_back(p);
	}
	return ports;
}

vector<AudioPort> AudioSystemIOS::getOutputs() {
	vector<AudioPort> ports;

	NSArray<AVAudioSessionPortDescription *> *descs = [[AVAudioSession sharedInstance] availableInputs];
	for (AVAudioSessionPortDescription *desc in descs) {
		AudioPort p;
		NSLog(@"Port: %@", desc.portName);
		p.name = [desc.portName UTF8String];
		for (AVAudioSessionChannelDescription *chan in desc.channels) {
			NSLog(@"CHANNEL %@ , num %lu label: %u",
				  chan.channelName,
				  (unsigned long) chan.channelNumber,
				  (unsigned int) chan.channelLabel);
		}
		ports.push_back(p);
	}

	return ports;
}

//
//  AudioFile.cpp
//  MZGL
//
//  Created by Marek Bereza on 01/03/2019.
//  Copyright © 2019 Marek Bereza. All rights reserved.
//

#include "AudioFile.h"
#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
//#include <CoreAudio/CoreAudio.h>
#import <Foundation/Foundation.h>

#include <AudioToolbox/AudioToolbox.h>
#endif
bool AudioFile::load(std::string path, FloatBuffer &buff, int *outNumChannels, int *outSampleRate) {
#ifdef __APPLE__
	
	CFURLRef audioFileURL = CFURLCreateWithBytes (
												  NULL,
												  (const UInt8 *)path.c_str(),
												  path.size(),
												  kCFStringEncodingASCII,
												  NULL
												  );

	ExtAudioFileRef xafref = nullptr;
	
	OSStatus err = noErr;
	err = ExtAudioFileOpenURL(audioFileURL, &xafref);
	
	if(err!=noErr) {
		printf("Couldn't load file\n");
		return false;
	}
	
	AudioStreamBasicDescription inputFormat;
	
	UInt32 propertySize = sizeof(inputFormat);
	err = ExtAudioFileGetProperty(xafref, kExtAudioFileProperty_FileDataFormat, &propertySize, &inputFormat);
	
	auto sampleRate = inputFormat.mSampleRate;
//	*outSampleRate = (unsigned) inputFormat.mSampleRate;
	*outNumChannels = (unsigned) inputFormat.mChannelsPerFrame;
	
//	printf("sr: %d    numchans: %d\n",*outSampleRate, *outNumChannels);
	
	AudioStreamBasicDescription audioFormat;
	audioFormat.mSampleRate = sampleRate;
	audioFormat.mFormatID = kAudioFormatLinearPCM;
	audioFormat.mFormatFlags = kLinearPCMFormatFlagIsFloat;
	audioFormat.mBitsPerChannel = sizeof(Float32) * 8;
	audioFormat.mChannelsPerFrame = *outNumChannels; // set this to 2 for stereo
	audioFormat.mBytesPerFrame = audioFormat.mChannelsPerFrame * sizeof(Float32);
	audioFormat.mFramesPerPacket = 1;
	audioFormat.mBytesPerPacket = audioFormat.mFramesPerPacket * audioFormat.mBytesPerFrame;
	
	// 3) Apply audio format to my Extended Audio File
	err = ExtAudioFileSetProperty(xafref,
								  kExtAudioFileProperty_ClientDataFormat,
								  sizeof (AudioStreamBasicDescription),
								  &audioFormat);
	if(err!=noErr) {
		printf("Couldn't set client data format on input ext file\n");
		return false;
	}
	
	UInt32 outputBufferSize = 32 * 1024; // 32 KB
	UInt32 sizePerPacket = audioFormat.mBytesPerPacket;
	UInt32 packetsPerBuffer = outputBufferSize / sizePerPacket;
	UInt8 *outputBuffer = (UInt8 *)malloc(sizeof(UInt8 *) * outputBufferSize);
	AudioBufferList convertedData;
	convertedData.mNumberBuffers = 1;
	convertedData.mBuffers[0].mNumberChannels = audioFormat.mChannelsPerFrame;
	convertedData.mBuffers[0].mDataByteSize = outputBufferSize;
	convertedData.mBuffers[0].mData = outputBuffer;
	
	bool done = false;
#pragma warning how many channels!!!!?!?!?!?!!?!?!
//	printf("#pragma warning how many channels!!!!?!?!?!?!!?!?!\n");
	while(!done) {
		
		UInt32 frameCount = packetsPerBuffer;
		
		err = ExtAudioFileRead(xafref, &frameCount, &convertedData);
		
		float *b = (float*)convertedData.mBuffers[0].mData;
		buff.insert(buff.end(), &b[0],&b[frameCount**outNumChannels]);
		
		if(err!=noErr) {
			printf("Error reading buffer\n");
			free(outputBuffer);
			return false;
		}
		if(frameCount!=packetsPerBuffer) {
			done = true;
		}
	}
	free(outputBuffer);
#endif
	if(outSampleRate!=nullptr) {
		*outSampleRate = sampleRate;
	}

	return true;
}


bool AudioFile::loadResampled(std::string path, FloatBuffer &buff, int newSampleRate, int *outNumChannels) {
//	CFURLRef audioFileURL = CFURLCreateWithBytes (
//												  NULL,
//												  (const UInt8 *)path.c_str(),
//												  path.size(),
//												  kCFStringEncodingUTF8,
//												 // kCFStringEncodingASCII,
//												  NULL
//												  );
//
	
	CFURLRef audioFileURL = (__bridge CFURLRef)[NSURL fileURLWithPath:[NSString stringWithUTF8String: path.c_str()]];

//	{
//		// debug
//
//		std::string str(CFStringGetCStringPtr(CFURLGetString(audioFileURL),kCFStringEncodingUTF8));
//
//	}
	ExtAudioFileRef xafref = nullptr;
	
	OSStatus err = noErr;
	err = ExtAudioFileOpenURL(audioFileURL, &xafref);
	
	if(err!=noErr) {
		printf("Couldn't load file\n");
		return false;
	}
	
	AudioStreamBasicDescription inputFormat;
	
	UInt32 propertySize = sizeof(inputFormat);
	err = ExtAudioFileGetProperty(xafref, kExtAudioFileProperty_FileDataFormat, &propertySize, &inputFormat);
	
	
	//*outSampleRate = (unsigned) inputFormat.mSampleRate;
	*outNumChannels = (unsigned) inputFormat.mChannelsPerFrame;
	
	printf("sr: %d    numchans: %d\n", newSampleRate, *outNumChannels);
	
	
	
	
	AudioStreamBasicDescription audioFormat;
	audioFormat.mSampleRate = newSampleRate;
	audioFormat.mFormatID = kAudioFormatLinearPCM;
	audioFormat.mFormatFlags = kLinearPCMFormatFlagIsFloat;
	audioFormat.mBitsPerChannel = sizeof(Float32) * 8;
	audioFormat.mChannelsPerFrame = *outNumChannels; // set this to 2 for stereo
	audioFormat.mBytesPerFrame = audioFormat.mChannelsPerFrame * sizeof(Float32);
	audioFormat.mFramesPerPacket = 1;
	audioFormat.mBytesPerPacket = audioFormat.mFramesPerPacket * audioFormat.mBytesPerFrame;
	
	// 3) Apply audio format to my Extended Audio File
	err = ExtAudioFileSetProperty(xafref,
								  kExtAudioFileProperty_ClientDataFormat,
								  sizeof (AudioStreamBasicDescription),
								  &audioFormat);
	if(err!=noErr) {
		printf("Couldn't set client data format on input ext file\n");
		return 0;
	}
	
	UInt32 outputBufferSize = 32 * 1024; // 32 KB
	UInt32 sizePerPacket = audioFormat.mBytesPerPacket;
	UInt32 packetsPerBuffer = outputBufferSize / sizePerPacket;
	UInt8 *outputBuffer = (UInt8 *)malloc(sizeof(UInt8 *) * outputBufferSize);
	AudioBufferList convertedData;
	convertedData.mNumberBuffers = 1;
	convertedData.mBuffers[0].mNumberChannels = audioFormat.mChannelsPerFrame;
	convertedData.mBuffers[0].mDataByteSize = outputBufferSize;
	convertedData.mBuffers[0].mData = outputBuffer;
	
	
	bool done = false;
#pragma warning how many channels!!!!?!?!?!?!!?!?!
	//printf("#pragma warning how many channels!!!!?!?!?!?!!?!?!\n");
	while(!done) {
		
		UInt32 frameCount = packetsPerBuffer;
		
		err = ExtAudioFileRead(xafref, &frameCount, &convertedData);
		
		
		float *b = (float*)convertedData.mBuffers[0].mData;
		buff.insert(buff.end(), &b[0],&b[frameCount**outNumChannels]);
		
		
		if(err!=noErr) {
			printf("Error reading buffer\n");
			free(outputBuffer);
			return false;
		}
		if(frameCount!=packetsPerBuffer) {
			done = true;
		}
		
	}
	free(outputBuffer);
	return true;
}

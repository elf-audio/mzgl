/*
 *  AVFoundationVideoGrabber.h
 */

#pragma once

#include <vector>
#include <string>
#include "Texture.h"
#import <CoreGraphics/CoreGraphics.h>

class AVFoundationVideoGrabber {
public:
	AVFoundationVideoGrabber();
	~AVFoundationVideoGrabber();

	void clear();
	void setCaptureRate(int capRate);

	bool initGrabber(int w, int h);
	bool isInitialized();
	void updatePixelsCB(CGImageRef &ref);

	void update();

	bool isFrameNew();

	std::vector<std::string> listDevices();
	void setDevice(int deviceID);
	//bool setPixelFormat(ofPixelFormat PixelFormat);
	//ofPixelFormat getPixelFormat();

	unsigned char *getPixels() { return pixels; }
	float getWidth() { return width; }
	float getHeight() { return height; }
	// GLint
	int32_t internalGlDataType;
	unsigned char *pixels;
	bool newFrame;
	bool bLock;

	int width, height;

protected:
	int device;
	bool bIsInit;
	bool bHavePixelsChanged;

	int fps;
	TextureRef tex;
	// GLubyte
	uint8_t *pixelsTmp;
};

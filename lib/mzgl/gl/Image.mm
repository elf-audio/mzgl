//
//  Image.cpp
//  samploid
//
//  Created by Marek Bereza on 07/02/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//
#include <ImageIO/ImageIO.h>
#include "Image.h"
#include <Foundation/Foundation.h>
#include <TargetConditionals.h>
#if TARGET_OS_IOS
#import <MobileCoreServices/MobileCoreServices.h>
#endif
#include "log.h"

using namespace std;


const CFStringRef Image__getFormatForPath(string path) {
	int dotIndex = (int)path.rfind(".");
	if(dotIndex==-1) {
		return NULL;
	}
	
	string ext = path.substr(dotIndex+1);
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	
	
	if(ext=="png") {
		return kUTTypePNG;
	} else if(ext=="jpg" || ext=="jpeg") {
		return kUTTypeJPEG;
	} else if(ext=="tif" || ext=="tiff") {
		return kUTTypeTIFF;
	} else if(ext=="pic" || ext=="pict") {
		return kUTTypePICT;
	} else if(ext=="bmp") {
		return kUTTypeBMP;
	} else if(ext=="gif") {
		return kUTTypeGIF;
	}
	return kUTTypePNG;
}
void Image__deleteData ( void *info, const void *data, size_t size ) {
	//delete [] info;
}


bool Image::save(string path, uint8_t *data, int width, int height, int numChannels, int bytesPerChannel, bool isFloat) {

	int bitsPerPixel = 8 * bytesPerChannel*numChannels;
	int bytesPerRow = width * bytesPerChannel*numChannels;

	CGDataProviderRef provider = CGDataProviderCreateWithData(data, data, width*height*numChannels*bytesPerChannel, &Image__deleteData);

	CGColorSpaceRef space;

	if(numChannels==1) {
		space = CGColorSpaceCreateDeviceGray();
	} else {
		space = CGColorSpaceCreateDeviceRGB();
	}

	CGBitmapInfo info;
	
	
	CGBitmapInfo alpha = 0;
	if(numChannels==4) {
		alpha = kCGImageAlphaLast;
	}
	if(isFloat) info = kCGBitmapByteOrder32Host | kCGBitmapFloatComponents | alpha;
	else if(bytesPerChannel==1) info = kCGBitmapByteOrderDefault | alpha;
	else if(bytesPerChannel==2) info = kCGBitmapByteOrder16Host | alpha;
	else if (bytesPerChannel==4) info = kCGBitmapByteOrder32Host | alpha;
	

	
	
	
	

	CGImageRef img = CGImageCreate(width,                         // width
								   height,                         // height
								   bytesPerChannel*8,         // bitsPerComponent
								   bitsPerPixel,               // bitsPerPixel
								   bytesPerRow,                // bytesPerRow
								   space,                      // colorspace
								   info,  // bitmapInfo
								   provider,                   // CGDataProvider
								   NULL,                       // decode array
								   false,                         // shouldInterpolate
								   kCGRenderingIntentDefault); // intent


	CGColorSpaceRelease(space);

	// this crashed my app
	CGDataProviderRelease(provider);

	// use the created CGImage
	NSString *pathNSString = [NSString stringWithFormat:@"%s", path.c_str()];
	NSURL *fileURL = [NSURL fileURLWithPath:pathNSString];


	const CFStringRef format = Image__getFormatForPath(path);
	if(format!=NULL) {
		CGImageDestinationRef dr = CGImageDestinationCreateWithURL((__bridge CFURLRef)fileURL, format , 1, NULL);
		if(dr!=NULL) {
			CGImageDestinationAddImage(dr, img, NULL);
			CGImageDestinationFinalize(dr);
			CFRelease(dr);
		} else {
			Log::e() << "Error! ImageIO: CGImageDestinationRef was NULL!";
		}
	} else {
		Log::e() << "Error! couldn't detect file type for path: " << path;
	}


	CGImageRelease(img);
	return true;
}



bool Image::load(string path, vector<uint8_t> &outData, int &outWidth, int &outHeight, int &outNumChannels, int &outBytesPerChannel, bool &outIsFloat) {
	
	
	CFStringRef       myKeys[1];
	CFTypeRef         myValues[1];
	
	CFDictionaryRef   myOptions = NULL;
	
	myKeys[0] = kCGImageSourceShouldCache;
	myValues[0] = (CFTypeRef)kCFBooleanFalse;
	
	
	myOptions = CFDictionaryCreate(NULL, (const void **) myKeys,
								   (const void **) myValues, 1,
								   &kCFTypeDictionaryKeyCallBacks,
								   & kCFTypeDictionaryValueCallBacks);
	
	
	
	
	NSString *pathNSString = [NSString stringWithFormat:@"%s", path.c_str()];
	NSURL *fileURL = [NSURL fileURLWithPath:pathNSString];
	CGImageSourceRef src = CGImageSourceCreateWithURL((CFURLRef)fileURL, myOptions);
	
	
	if(src==NULL) {
		Log::e() << "Error: couldn't create CGImageSourceRef for file " << path;
		return false;
	}
	
	
	CGImageRef img = CGImageSourceCreateImageAtIndex(src, 0, myOptions);
	if(img==nullptr) {
		return false;
	}
	CFRelease(myOptions);
	outWidth = (int)CGImageGetWidth(img);
	outHeight = (int)CGImageGetHeight(img);
	CFRelease(src);
	
	int bitsPerComponent = (int)CGImageGetBitsPerComponent(img);
	int bitsPerPixel = (int)CGImageGetBitsPerPixel(img);
	int bytesPerRow = (int)CGImageGetBytesPerRow(img);
	
	outNumChannels = bitsPerPixel / bitsPerComponent;
	
	int bytesPerPixel = bitsPerPixel / 8;
	
	
	if(bytesPerPixel*outWidth != bytesPerRow) {
		Log::e() << "bytesPerPixel*width != bytesPerRow - ImageIO doesn't support this at the moment, soz.";
		return false;
	}
	
	
	// work out type
	CGBitmapInfo info = CGImageGetBitmapInfo(img);
	outBytesPerChannel = bytesPerPixel/outNumChannels;
	
	if(info & kCGBitmapFloatComponents) {
		outIsFloat = true;
	} else {
		outIsFloat = false;
	}
	//CFDataRef pix = CopyImagePixels(img);
	//CFDataRef CopyImagePixels(CGImageRef inImage) {     return CGDataProviderCopyData(CGImageGetDataProvider(inImage)); }
	CFDataRef pix = CGDataProviderCopyData(CGImageGetDataProvider(img));
	if(pix==nullptr) {
		CGImageRelease(img);
		return false;
	}
	
	outData.resize(outWidth*outHeight*outNumChannels*outBytesPerChannel);
	CFDataGetBytes(pix, CFRangeMake(0,CFDataGetLength(pix)), outData.data());
	
	CGImageRelease(img);
	CFRelease(pix);
	return true;
}




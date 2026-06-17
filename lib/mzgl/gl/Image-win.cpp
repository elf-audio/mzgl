//
//  Image.cpp
//  samploid
//
//  Created by Marek Bereza on 07/02/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#include "Image.h"

#include <cstring>
//#include "picopng.hpp"

// The stb_image READ implementation is compiled on every platform: Image.cpp's
// loadPngFromData() (PNG-from-memory) uses stbi_* everywhere, including Apple.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Image::load/save (from a file path) use stb_image here on non-Apple platforms.
// On Apple they are provided by Image.mm via ImageIO/CoreGraphics, so this file
// must NOT also define them there (it would be a duplicate symbol). The stb write
// implementation is likewise only needed for the stb-based save below.
#if !defined(__APPLE__)

#	define STB_IMAGE_WRITE_IMPLEMENTATION
#	include "stb_image_write.h"

using namespace std;

bool Image::save(
	const string &path, uint8_t *data, int width, int height, int numChannels, int bytesPerChannel, bool isFloat) {
	if (path.find(".png") != -1) {
		return stbi_write_png(path.c_str(), width, height, numChannels, data, width * numChannels);
	} else if (path.find(".bmp") != -1) {
		return stbi_write_bmp(path.c_str(), width, height, numChannels, data);
	} else if (path.find(".tga") != -1) {
		return stbi_write_tga(path.c_str(), width, height, numChannels, data);
	} else if (path.find(".jpg") != -1) {
		int jpgQuality = 95;
		return stbi_write_jpg(path.c_str(), width, height, numChannels, data, jpgQuality);
	}
	return false;
}

bool Image::load(const string &path,
				 vector<uint8_t> &outData,
				 int &outWidth,
				 int &outHeight,
				 int &outNumChannels,
				 int &outBytesPerChannel,
				 bool &outIsFloat) {
	unsigned char *image = stbi_load(path.c_str(), &outWidth, &outHeight, &outNumChannels, STBI_default);
	if (image == nullptr) return false;
	outBytesPerChannel = 1;
	// Defend against int overflow on a malformed PNG with huge declared
	// dimensions. `outWidth * outHeight * outNumChannels` in plain `int` wraps
	// past ~2.1 GB, leaving outData smaller than the texture upload below
	// expects and crashing the GPU driver inside glTexImage2D.
	if (outWidth <= 0 || outHeight <= 0 || outNumChannels <= 0 || outNumChannels > 4) {
		stbi_image_free(image);
		return false;
	}
	const size_t totalBytes = static_cast<size_t>(outWidth)
							  * static_cast<size_t>(outHeight)
							  * static_cast<size_t>(outNumChannels);
	outData.resize(totalBytes);
	memcpy(outData.data(), image, totalBytes);
	stbi_image_free(image);
	return true;
}

#endif // !defined(__APPLE__)

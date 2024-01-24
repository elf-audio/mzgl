//
//  Image.cpp
//  samploid
//
//  Created by Marek Bereza on 07/02/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#include <mzgl/gl/Image.h>

#include <cstring>
//#include "picopng.hpp"
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_write.h"

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
	//    printf("%d %d %d\n", outWidth, outHeight, outNumChannels);
	outData.resize(outWidth * outHeight * outNumChannels);
	memcpy(outData.data(), image, outWidth * outHeight * outNumChannels);
	stbi_image_free(image);
	return true;
}

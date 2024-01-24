//
//  Image.cpp
//  samploid
//
//  Created by Marek Bereza on 07/02/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#include "Image.h"

#include <mzgl/util/log.h>
#include "stb_image.h"

#include <algorithm>

#include <cstring>
#include <fsystem/fsystem.h>

Image::Image(const std::string &filePath) {
	if (!load(filePath)) {
		throw std::runtime_error("Can't load file at " + filePath + " cwd is: " + fs::current_path().string());
	}
}
bool Image::loadPngFromData(const std::vector<uint8_t> &inData,
							std::vector<uint8_t> &outData,
							int &outWidth,
							int &outHeight,
							int &outNumChannels,
							int &outBytesPerChannel,
							bool &outIsFloat) {
	unsigned char *image = stbi_load_from_memory(
		inData.data(), (int) inData.size(), &outWidth, &outHeight, &outNumChannels, STBI_default);

	outBytesPerChannel = 1;
	//  printf("%d %d %d\n", outWidth, outHeight, outNumChannels);
	outData.resize(outWidth * outHeight * outNumChannels);
	memcpy(outData.data(), image, outWidth * outHeight * outNumChannels);
	stbi_image_free(image);
	return true;
}

void Image::flipVertical() {
	int n = bytesPerChannel * numChannels;
	std::vector<uint8_t> pixel(n);
	for (int y = 0; y < height / 2; y++) {
		for (int x = 0; x < width; x++) {
			int upperPos = (x + (y * width)) * n;
			int lowerPos = (x + (height - y - 1) * width) * n;
			// temporarily copy upper pixel
			memcpy(pixel.data(), &data[upperPos], n);

			// copy lower pixel to upper pixel position
			memcpy(&data[upperPos], &data[lowerPos], n);

			// copy upper pixel in temp storage to lower pixel position
			memcpy(&data[lowerPos], pixel.data(), n);
		}
	}
}

void rgb2rgba(std::vector<uint8_t> &d) {
	int sz3 = (int) d.size() / 3;
	d.resize(sz3 * 4);
	for (int i = sz3 - 1; i >= 0; i--) {
		d[i * 4]	 = d[i * 3];
		d[i * 4 + 1] = d[i * 3 + 1];
		d[i * 4 + 2] = d[i * 3 + 2];
		d[i * 4]	 = 255;
	}
}
void rgba2rgb(std::vector<uint8_t> &d) {
	auto sz4 = d.size() / 4;
	for (int i = 0; i < sz4; i++) {
		d[i * 3]	 = d[i * 4];
		d[i * 3 + 1] = d[i * 4 + 1];
		d[i * 3 + 2] = d[i * 4 + 2];
	}
	d.resize(sz4 * 3);
}

//----------------------------------------------------------------------

float bicubicInterpolate(const float *patch, float x, float y, float x2, float y2, float x3, float y3) {
	// adapted from http://www.paulinternet.nl/?page=bicubic
	// Note that this code can produce values outside of 0...255, due to cubic overshoot.
	// The ofClamp() prevents this from happening.

	const float p00 = patch[0];
	const float p10 = patch[4];
	const float p20 = patch[8];
	const float p30 = patch[12];

	const float p01 = patch[1];
	const float p11 = patch[5];
	const float p21 = patch[9];
	const float p31 = patch[13];

	const float p02 = patch[2];
	const float p12 = patch[6];
	const float p22 = patch[10];
	const float p32 = patch[14];

	const float p03 = patch[3];
	const float p13 = patch[7];
	const float p23 = patch[11];
	const float p33 = patch[15];

	const float a00 = p11;
	const float a01 = -p10 + p12;
	const float a02 = 2.0f * p10 - 2.0f * p11 + p12 - p13;
	const float a03 = -p10 + p11 - p12 + p13;
	const float a10 = -p01 + p21;
	const float a11 = p00 - p02 - p20 + p22;
	const float a12 = -2.0f * p00 + 2.0f * p01 - p02 + p03 + 2.0f * p20 - 2.0f * p21 + p22 - p23;
	const float a13 = p00 - p01 + p02 - p03 - p20 + p21 - p22 + p23;
	const float a20 = 2.0f * p01 - 2.0f * p11 + p21 - p31;
	const float a21 = -2.0f * p00 + 2.0f * p02 + 2.0f * p10 - 2.0f * p12 - p20 + p22 + p30 - p32;
	const float a22 = 4 * p00 - 4 * p01 + 2.0f * p02 - 2.0f * p03 - 4 * p10 + 4 * p11 - 2.0f * p12 + 2.0f * p13
					  + 2.0f * p20 - 2.0f * p21 + p22 - p23 - 2.0f * p30 + 2.0f * p31 - p32 + p33;
	const float a23 = -2.0f * p00 + 2.0f * p01 - 2.0f * p02 + 2.0f * p03 + 2.0f * p10 - 2.0f * p11 + 2.0f * p12
					  - 2.0f * p13 - p20 + p21 - p22 + p23 + p30 - p31 + p32 - p33;
	const float a30 = -p01 + p11 - p21 + p31;
	const float a31 = p00 - p02 - p10 + p12 + p20 - p22 - p30 + p32;
	const float a32 = -2.0f * p00 + 2.0f * p01 - p02 + p03 + 2.0f * p10 - 2.0f * p11 + p12 - p13 - 2.0f * p20
					  + 2.0f * p21 - p22 + p23 + 2.0f * p30 - 2.0f * p31 + p32 - p33;
	const float a33 =
		p00 - p01 + p02 - p03 - p10 + p11 - p12 + p13 + p20 - p21 + p22 - p23 - p30 + p31 - p32 + p33;

	const float out = a00 + a01 * y + a02 * y2 + a03 * y3 + a10 * x + a11 * x * y + a12 * x * y2 + a13 * x * y3
					  + a20 * x2 + a21 * x2 * y + a22 * x2 * y2 + a23 * x2 * y3 + a30 * x3 + a31 * x3 * y
					  + a32 * x3 * y2 + a33 * x3 * y3;

	return std::min(static_cast<size_t>(255), std::max(static_cast<size_t>(out), static_cast<size_t>(0)));
}

// SO SLOW - maybe use something like this instead? https://blog.demofox.org/2015/08/15/resizing-images-with-bicubic-interpolation/

void Image::resize(int newWidth, int newHeight) {
	//	avir :: CImageResizer<> ImageResizer( bytesPerChannel*8 );
	//	vector<uint8_t> outputBuffer(newWidth * newHeight * bytesPerChannel*numChannels);
	//	ImageResizer.resizeImage(data.data(), width, height, 0, outputBuffer.data(), newWidth, newHeight, numChannels, 0 );
	//	data = outputBuffer;
	//	width = newWidth;
	//	height = newHeight;
	//}

	// pinched from openFrameworks
	size_t srcWidth		 = width;
	size_t srcHeight	 = height;
	size_t dstWidth		 = newWidth;
	size_t dstHeight	 = newHeight;
	size_t bytesPerPixel = bytesPerChannel * numChannels;

	std::vector<uint8_t> outputBuffer(newWidth * newHeight * bytesPerChannel * numChannels);

	uint8_t *dstPixels		   = outputBuffer.data();
	const int BICUBIC		   = 0;
	const int NEAREST_NEIGHBOR = 1;
	int interpMethod		   = BICUBIC;
	switch (interpMethod) {
			//----------------------------------------
		case NEAREST_NEIGHBOR: {
			size_t dstIndex	 = 0;
			float srcxFactor = (float) srcWidth / dstWidth;
			float srcyFactor = (float) srcHeight / dstHeight;
			float srcy		 = 0.5;
			for (size_t dsty = 0; dsty < dstHeight; dsty++) {
				float srcx		= 0.5;
				size_t srcIndex = static_cast<size_t>(srcy) * srcWidth;
				for (size_t dstx = 0; dstx < dstWidth; dstx++) {
					size_t pixelIndex = static_cast<size_t>(srcIndex + srcx) * bytesPerPixel;
					for (size_t k = 0; k < bytesPerPixel; k++) {
						dstPixels[dstIndex] = data[pixelIndex];
						dstIndex++;
						pixelIndex++;
					}
					srcx += srcxFactor;
				}
				srcy += srcyFactor;
			}
		} break;

			//----------------------------------------

			//----------------------------------------
		case BICUBIC:
			float px1, py1;
			float px2, py2;
			float px3, py3;

			float srcColor = 0;
			float interpCol;
			size_t patchRow;
			size_t patchIndex;
			float patch[16];

			size_t srcRowBytes = srcWidth * bytesPerPixel;
			size_t loIndex	   = (srcRowBytes) + 1;
			size_t hiIndex	   = (srcWidth * srcHeight * bytesPerPixel) - (srcRowBytes) -1;

			for (size_t dsty = 0; dsty < dstHeight; dsty++) {
				for (size_t dstx = 0; dstx < dstWidth; dstx++) {
					size_t dstIndex0 = (dsty * dstWidth + dstx) * bytesPerPixel;
					float srcxf		 = srcWidth * (float) dstx / (float) dstWidth;
					float srcyf		 = srcHeight * (float) dsty / (float) dstHeight;
					size_t srcx		 = static_cast<size_t>(std::min(srcWidth - 1, static_cast<size_t>(srcxf)));
					size_t srcy		 = static_cast<size_t>(std::min(srcHeight - 1, static_cast<size_t>(srcyf)));
					size_t srcIndex0 = (srcy * srcWidth + srcx) * bytesPerPixel;

					px1 = srcxf - srcx;
					py1 = srcyf - srcy;
					px2 = px1 * px1;
					px3 = px2 * px1;
					py2 = py1 * py1;
					py3 = py2 * py1;

					for (size_t k = 0; k < bytesPerPixel; k++) {
						size_t dstIndex = dstIndex0 + k;
						size_t srcIndex = srcIndex0 + k;

						for (size_t dy = 0; dy < 4; dy++) {
							patchRow = srcIndex + ((dy - 1) * srcRowBytes);
							for (size_t dx = 0; dx < 4; dx++) {
								patchIndex = patchRow + (dx - 1) * bytesPerPixel;
								if ((patchIndex >= loIndex) && (patchIndex < hiIndex)) {
									srcColor = data[patchIndex];
								}
								patch[dx * 4 + dy] = srcColor;
							}
						}

						interpCol			= (uint8_t) bicubicInterpolate(patch, px1, py1, px2, py2, px3, py3);
						dstPixels[dstIndex] = interpCol;
					}
				}
			}
			break;
	}
	data   = outputBuffer;
	width  = newWidth;
	height = newHeight;
	Log::d() << "Done resize";
}

//
//  Image.hpp
//  samploid
//
//  Created by Marek Bereza on 07/02/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once
#include <stdlib.h>
#include <string>
#include <vector>
#include <memory>


class Image;
typedef std::shared_ptr<Image> ImageRef;

class Image {
public:
	

	static ImageRef create(std::string path) {
		return ImageRef(new Image(path));
	}
	
	static ImageRef create(int w, int h, int numChannels = 4, int bytesPerChannel = 1, bool isFloat = false) {
		return ImageRef(new Image(w, h, numChannels, bytesPerChannel, isFloat));
	}
	
	static bool save(std::string path, uint8_t *data, int width, int height, int numChannels, int bytesPerChannel, bool isFloat = false);
	static bool load(std::string path, std::vector<uint8_t> &outData, int &outWidth, int &outHeight, int &outNumChannels, int &outBytesPerChannel, bool &outIsFloat);

	static bool loadPngFromData(const std::vector<uint8_t> &inData, std::vector<uint8_t> &outData, int &outWidth, int &outHeight, int &outNumChannels, int &outBytesPerChannel, bool &outIsFloat);


	bool load(std::string path) {
		return load(path, data, width, height, numChannels, bytesPerChannel, isFloat);
	}
	void save(std::string path) {
		save(path, data.data(), width, height, numChannels, bytesPerChannel, isFloat);
	}
	
	void flipVertical();
	void resize(int newWidth, int newHeight);
	std::vector<uint8_t> data;
	
	int width = 0;
	int height = 0;
	int numChannels = 3;
	int bytesPerChannel = 1;
	
	bool isFloat = false;
	
private:
	
	Image(std::string filePath) {
		load(filePath);
	}
	Image() {}
	
	// allocates an image with parameters.
	Image(int w, int h, int numChannels, int bytesPerChannel, bool isFloat) :
	width(w), height(h), numChannels(numChannels), bytesPerChannel(bytesPerChannel), isFloat(isFloat) {
		data.resize(w * h * numChannels * bytesPerChannel);
	}
};
void rgb2rgba(std::vector<uint8_t> &d);
void rgba2rgb(std::vector<uint8_t> &d);

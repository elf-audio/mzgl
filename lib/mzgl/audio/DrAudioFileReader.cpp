#include "DrAudioFileReader.h"

/////////////////////////////////////////
// #if defined(__linux__) && !defined(__android__)
//  #define DR_WAV_IMPLEMENTATION
// #define DR_FLAC_IMPLEMENTATION
// #define DR_MP3_IMPLEMENTATION
// #endif

#include "dr_wav.h"
#include <algorithm>
using namespace std;

class DrWavFileReader : public DrAudioFileReader {
public:
	virtual bool open(string path) override {
		if(!drwav_init_file(&infile, path.c_str(), nullptr)) {
			printf("Can't open file %s for reading\n", path.c_str());
			return false;
		}
		numChannels = infile.fmt.channels;
		return true;
	}

	virtual uint32_t getNumChannels() override {
		return numChannels;
	}

	virtual uint32_t getSampleRate() override {
		return infile.fmt.sampleRate;
	}

	virtual uint32_t read(vector<float> &outSamples) override {
		uint32_t framesRead = drwav_read_pcm_frames_f32(&infile, outSamples.size()/numChannels, outSamples.data());
		if(framesRead!=outSamples.size()/numChannels) outSamples.resize(framesRead*numChannels);
		return framesRead;
	}

	virtual void close() override {
		drwav_uninit(&infile);
	}

private:
	drwav infile;
	int numChannels;
};




// 
#include "dr_flac.h"

class DrFlacFileReader : public DrAudioFileReader {
public:
	virtual bool open(string path) override {
		infile = drflac_open_file(path.c_str(), nullptr);
		if(infile==nullptr) {
			printf("Can't open file %s for reading\n", path.c_str());
			return false;
		}
		return true;
	}

	virtual uint32_t getNumChannels() override {
		return infile->channels;
	}

	virtual uint32_t getSampleRate() override {
		return infile->sampleRate;
	}

	virtual uint32_t read(vector<float> &outSamples) override {
		uint32_t framesRead = drflac_read_pcm_frames_f32(infile, outSamples.size()/getNumChannels(), outSamples.data());
		if(framesRead!=outSamples.size()/getNumChannels()) outSamples.resize(framesRead*getNumChannels());
		return framesRead;
	}

	virtual void close() override {
		if(infile!=nullptr) drflac_close(infile);
	}

private:
	drflac *infile = nullptr;
};






// 
#include "dr_mp3.h"

class DrMp3FileReader : public DrAudioFileReader {
public:
	virtual bool open(string path) override {

		if (!drmp3_init_file(&infile, path.c_str(), nullptr)) {			
			printf("Can't open file %s for reading\n", path.c_str());
			return false;
		}
		return true;
	}

	virtual uint32_t getNumChannels() override {
		return infile.channels;
	}

	virtual uint32_t getSampleRate() override {
		return infile.sampleRate;
	}

	virtual uint32_t read(vector<float> &outSamples) override {
		uint32_t framesRead = drmp3_read_pcm_frames_f32(&infile, outSamples.size()/getNumChannels(), outSamples.data());
		if(framesRead!=outSamples.size()/getNumChannels()) outSamples.resize(framesRead*getNumChannels());
		return framesRead;
	}

	virtual void close() override {
		drmp3_uninit(&infile);
	}

private:
	drmp3 infile;

};






//////////////////////////////////////
bool DrAudioFileReader::open(string path) {
	int dotPos = path.rfind(".");
	if(dotPos==-1) {
		printf("Error: no file extension on %s\n", path.c_str());
		return false;
	}
	string ext = path.substr(dotPos+1);
	std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });
	if(ext=="wav") {
		impl = unique_ptr<DrAudioFileReader>(new DrWavFileReader());
	} else if(ext=="flac") {
		impl = unique_ptr<DrAudioFileReader>(new DrFlacFileReader());
	} else if(ext=="mp3") {
		impl = unique_ptr<DrAudioFileReader>(new DrMp3FileReader());
	} else {
		printf("Can't handle that file format: %s\n", ext.c_str());
		return false;
	}

	return impl->open(path);
}

uint32_t DrAudioFileReader::read(vector<float> &outSamples) {
	if(impl!=nullptr) return impl->read(outSamples);

	// default behaviour
	outSamples.resize(0);
	return 0;
}

uint32_t DrAudioFileReader::getNumChannels() {
	if(impl!=nullptr) return impl->getNumChannels();
	return 0;
}

uint32_t DrAudioFileReader::getSampleRate() {
	if(impl!=nullptr) return impl->getSampleRate();
	return 0;
}

void DrAudioFileReader::close() {
	if(impl!=nullptr) impl->close();
}





/////////////////////////////////

void DrAudioFileReader::writeWavFile(const string &path, vector<float> &data, int numChannels, int sampleRate) {
	// TODO: stereo
	drwav outfile;
	drwav_data_format fmt;
	fmt.container = drwav_container_riff;
	fmt.format = DR_WAVE_FORMAT_PCM;
	fmt.channels = numChannels;
	fmt.sampleRate = sampleRate;
	fmt.bitsPerSample = 16;


	vector<drwav_int16> pcmOut(data.size());

	drwav_f32_to_s16(pcmOut.data(), data.data(), data.size());

	// TODO: STEREO!!
	drwav_init_write_sequential_pcm_frames(&outfile, &fmt, data.size()/numChannels, nullptr, nullptr, nullptr);
	drwav_init_file_write(&outfile, path.c_str(), &fmt, nullptr);
	drwav_write_pcm_frames(&outfile, pcmOut.size()/numChannels, pcmOut.data());
	drwav_uninit(&outfile);
}

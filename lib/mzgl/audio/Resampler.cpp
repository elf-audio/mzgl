
#include "Resampler.h"

#include "speex_resampler.h"
#include <stdio.h>

using namespace std;
bool Resampler::init(int numChannels, int inSampleRate, int outSampleRate, int quality) {
	ratio			  = outSampleRate / (double) inSampleRate;
	int error		  = 0;
	this->numChannels = numChannels;
	resampler		  = speex_resampler_init(numChannels, inSampleRate, outSampleRate, quality, &error);
	if (error != 0) {
		printf("Got resampler error: %d\n", error);
		return false;
	}
	if (resampler == nullptr) {
		printf("resampler couldn't be created\n");
		return false;
	}

	error = speex_resampler_skip_zeros(resampler);
	if (error != 0) {
		printf("Could not skip zeros: %d\n", error);
		return false;
	}

	return true;
}

void Resampler::process(const vector<float> &ins, vector<float> &outs) {
	int outNumberOfSamples = (int) ((double) (ins.size() + 2) * ratio);
	if (outs.size() < outNumberOfSamples) outs.resize(outNumberOfSamples);

	spx_uint32_t inFrames  = ins.size() / numChannels;
	spx_uint32_t outFrames = outs.size() / numChannels;
	int error =
		speex_resampler_process_interleaved_float(resampler, ins.data(), &inFrames, outs.data(), &outFrames);
	if (outFrames * numChannels != outs.size()) outs.resize(outFrames * numChannels);
}

Resampler::~Resampler() {
	if (resampler != nullptr) {
		speex_resampler_destroy(resampler);
		resampler = nullptr;
	}
}


#include "App.h"
#include <cmath>
#include "mzAssert.h"

class MyApp : public App {
public:
	MyApp(Graphics &g)
		: App(g) {}

	float inputLevel = 0.f;

	void draw() override {
		g.clear(0);

		// set colour according to filter amount and input level
		g.setColor(1, filterAmt, inputLevel);

		// draw rectangle that looks like a volume meter
		g.drawRect(0, g.height, g.width, -g.height * inputLevel);

		g.setColor(1);
		g.drawTextCentred("HOVER MOUSE ON SCREEN TO SET FILTER", {g.width * 0.5f, 100.f});
		g.drawTextCentred("MAKE NOISE INTO MIC TO SEE LEVEL CHANGE", {g.width * 0.5f, 150.f});
	}

	// when you move the mouse it changes the "filterAmt"
	// p.s. touchOver is the same as hovering the mouse. There's actually no equivalent
	// in the touch world.
	void touchOver(float x, float y) override { filterAmt = mapf(x, 0, g.width, 0.001, 0.999, true); }

	void audioIn(float *samples, int numFrames, int nChans) override {
		float absMax = 0.f;
		// find the largest absolute max value in the buffer (left channel only)
		for (int i = 0; i < numFrames; i++) {
			float f = abs(samples[i * nChans]);
			if (f > absMax) absMax = f;
		}

		// change the curve to something more exponential
		absMax = sqrt(absMax);

		// bit of smoothing
		inputLevel = inputLevel * 0.9 + absMax * 0.1;
	}

	/** 
     * 
     * Gets called when the OS wants samples to play through the computers
     * audio output. Fill the "samples" buffer with audio data (e.g. float, -1 to +1)
     * 
     * @param numFrames - the number of frames - e.g. samples.length == numFrames * nChans
     * @param nChans - the number of channels, e.g. 1 for mono, 2 for stereo.
     * 
     * If nChans>1 (e.g. stereo or more channels) the data is interleaved
     * LRLRLRLRLR - one left sample then one right sample then one left, one right, etc.
     * a pair of LR in this case is a "frame"
     */
	void audioOut(float *samples, int numFrames, int nChans) override {
		mzAssert(nChans == 2); // we're assuming there are 2 outputs, e.g. stereo
		for (int i = 0; i < numFrames; i++) {
			// random white noise going between -0.25 and +0.25
			float s = randuf() * 0.25;

			// some sort of very basic low pass filter
			s = s * filterAmt + lastOut * (1.f - filterAmt);

			lastOut		   = s;
			samples[i * 2] = samples[i * 2 + 1] = s;
		}
	}
	float filterAmt = 0.5;
	float lastOut	= 0.f;
};

std::shared_ptr<AudioSystem> audioSystem = nullptr;
std::shared_ptr<App> instantiateApp(Graphics &g) {
	g.width	 = 800;
	g.height = 800;

	auto app = std::make_shared<MyApp>(g);

	audioSystem = std::make_shared<AudioSystem>();
	audioSystem->setBufferSize(256);
	audioSystem->setup(2, 2);
	audioSystem->bindToApp(app.get());
	audioSystem->start();
	return app;
}

#include "AudioBufferAdaptor.h"
#include "tests.h"
TEST_CASE("AudioRingBuffer_write", "[audio]") {
	AudioRingBuffer buffer(4);
	float data[4] = {1, 2, 3, 4};
	REQUIRE(buffer.write(data, 4) == 4);
	REQUIRE(buffer.available() == 4);

	float readData[4];
	REQUIRE(buffer.read(readData, 4) == 4);
	REQUIRE(buffer.available() == 0);
	REQUIRE(readData[0] == 1);
	REQUIRE(readData[1] == 2);
	REQUIRE(readData[2] == 3);
	REQUIRE(readData[3] == 4);
}

TEST_CASE("AudioRingBuffer_read", "[audio]") {
	AudioRingBuffer buffer(4);
	float data[4] = {1, 2, 3, 4};
	REQUIRE(buffer.write(data, 4) == 4);
	REQUIRE(buffer.available() == 4);

	float readData[4];
	REQUIRE(buffer.read(readData, 2) == 2);
	REQUIRE(buffer.available() == 2);
	REQUIRE(readData[0] == 1);
	REQUIRE(readData[1] == 2);

	REQUIRE(buffer.read(readData, 2) == 2);
	REQUIRE(buffer.available() == 0);
	REQUIRE(readData[0] == 3);
	REQUIRE(readData[1] == 4);
}

TEST_CASE("AudioRingBuffer_peek", "[audio]") {
	AudioRingBuffer buffer(4);
	float data[4] = {1, 2, 3, 4};
	REQUIRE(buffer.write(data, 4) == 4);
	REQUIRE(buffer.available() == 4);

	float readData[4];
	REQUIRE(buffer.peek(readData, 2) == 2);
	REQUIRE(buffer.available() == 4);
	REQUIRE(readData[0] == 1);
	REQUIRE(readData[1] == 2);

	REQUIRE(buffer.read(readData, 2) == 2);
	REQUIRE(buffer.available() == 2);
	REQUIRE(readData[0] == 1);
	REQUIRE(readData[1] == 2);

	REQUIRE(buffer.peek(readData, 2) == 2);
	REQUIRE(buffer.available() == 2);
	REQUIRE(readData[0] == 3);
	REQUIRE(readData[1] == 4);
}

class MockAudioIO : public AudioIO {
public:
	std::function<void(int)> inResult;
	std::function<void(int)> outResult;
	void audioIn(float *data, int frames, int chans) override { inResult(frames); }

	void audioOut(float *data, int frames, int chans) override { outResult(frames); }
};

TEST_CASE("AudioBufferAdaptor", "[audio]") {
	MockAudioIO mock;
	AudioBufferAdaptor adaptor(&mock, 16);
	REQUIRE(adaptor.getDestinationBufferSize() == 16);

	int inCalls	 = 0;
	int outCalls = 0;

	mock.inResult = [&](int frames) {
		REQUIRE(frames == 16);
		inCalls++;
	};
	mock.outResult = [&](int frames) {
		REQUIRE(frames == 16);
		outCalls++;
	};

	float randoBuff[4096];
	adaptor.audioIn(randoBuff, 16, 2);
	REQUIRE(inCalls == 0);

	adaptor.audioOut(randoBuff, 16, 2);
	REQUIRE(inCalls == 1);
	REQUIRE(outCalls == 1);

	adaptor.audioIn(randoBuff, 32, 2);
	REQUIRE(inCalls == 1);
	REQUIRE(outCalls == 1);

	adaptor.audioOut(randoBuff, 32, 2);
	REQUIRE(inCalls == 3);
	REQUIRE(outCalls == 3);

	adaptor.audioIn(randoBuff, 15, 2);
	// not working, work in progress
	//	adaptor.audioOut(randoBuff, 15, 2);
	//
	//	REQUIRE(inCalls == 3);
	//	REQUIRE(outCalls == 3);
	//	adaptor.audioIn(randoBuff, 2, 2);
	//	adaptor.audioOut(randoBuff, 2, 2);
	//
	//	REQUIRE(inCalls == 4);
	//	REQUIRE(outCalls == 4);
	//	adaptor.audioIn(randoBuff, 15, 2);
	//	adaptor.audioOut(randoBuff, 15, 2);
	//	REQUIRE(inCalls == 5);
	//	REQUIRE(outCalls == 5);
}
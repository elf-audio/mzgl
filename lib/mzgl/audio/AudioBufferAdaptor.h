#pragma once
#include "AudioSystem.h"
#include <vector>
// Simple ring buffer for audio samples
class AudioRingBuffer {
public:
	AudioRingBuffer(int capacity = 1024)
		: m_buffer(capacity)
		, m_capacity(capacity) {}

	// Write data to buffer (returns number of samples actually written)
	int write(const float *data, int count) {
		int written = 0;
		while (written < count && !isFull()) {
			m_buffer[m_writePos] = data[written];
			m_writePos			 = (m_writePos + 1) % m_capacity;
			m_size++;
			written++;
		}
		return written;
	}

	// Read data from buffer (returns number of samples actually read)
	int read(float *data, int count) {
		int read = 0;
		while (read < count && !isEmpty()) {
			data[read] = m_buffer[m_readPos];
			m_readPos  = (m_readPos + 1) % m_capacity;
			m_size--;
			read++;
		}
		return read;
	}

	// Peek at data without removing (returns number of samples peeked)
	int peek(float *data, int count) const {
		int readPos = m_readPos;
		int read	= 0;
		while (read < count && read < m_size) {
			data[read] = m_buffer[readPos];
			readPos	   = (readPos + 1) % m_capacity;
			read++;
		}
		return read;
	}

	// Available samples to read
	int available() const { return m_size; }

	bool isEmpty() const { return m_size == 0; }

	bool isFull() const { return m_size == m_capacity; }

	void clear() { m_readPos = m_writePos = m_size = 0; }

private:
	std::vector<float> m_buffer;
	int m_capacity;
	int m_readPos  = 0;
	int m_writePos = 0;
	int m_size	   = 0;
};

class AudioBufferAdaptor : public AudioIO {
public:
	// Constructor takes a target AudioIO implementation and fixed buffer size
	AudioBufferAdaptor(AudioIO *target, int fixedBufferSize = 256)
		: m_target(target)
		, destBuffSize(fixedBufferSize)
		, m_inputBuffer(fixedBufferSize * 4)
		, // Buffer size with some extra headroom
		m_outputBuffer(fixedBufferSize * 4) // Buffer size with some extra headroom
	{}

	void audioIn(float *data, int frames, int chans) override {
		inputChans = chans;
		m_inputBuffer.write(data, frames * chans);
	}

	// Generate output audio for OS with variable buffer size
	void audioOut(float *data, int frames, int chans) override {
		outputChans = chans;
		workingBuffer.resize(destBuffSize * maxNumChans);

		int outSamples	   = frames * chans;
		int samplesWritten = 0;

		while (samplesWritten < outSamples) {
			// If output buffer doesn't have enough data, get more from target
			if (m_outputBuffer.available() < outSamples - samplesWritten) {
				// Create a temporary buffer for target to fill
				m_inputBuffer.read(workingBuffer.data(), destBuffSize * inputChans);
				m_target->audioIn(workingBuffer.data(), destBuffSize, inputChans);

				// Get output from target
				m_target->audioOut(workingBuffer.data(), destBuffSize, chans);
				m_outputBuffer.write(workingBuffer.data(), destBuffSize * chans);
			}

			// Read available data from the output buffer (up to what we need)
			int samplesToRead = std::min(m_outputBuffer.available(), outSamples - samplesWritten);
			int samplesRead	  = m_outputBuffer.read(data + samplesWritten, samplesToRead);
			samplesWritten += samplesRead;
		}
	}

	// Get the fixed buffer size
	int getDestinationBufferSize() const { return destBuffSize; }

private:
	std::vector<float> workingBuffer;
	int maxNumChans	  = 2;
	int inputChans	  = 0;
	int outputChans	  = 0;
	AudioIO *m_target = nullptr;
	const int destBuffSize;

	AudioRingBuffer m_inputBuffer;
	AudioRingBuffer m_outputBuffer;
};

#include <string>
#include <vector>
#include <memory>
class DrAudioFileReader;

class DrAudioFileReader {
public:
	virtual bool open(std::string path);
	virtual uint32_t getNumChannels();
	virtual uint32_t read(std::vector<float> &outSamples);
	virtual uint32_t getSampleRate();
	virtual void close();
	virtual ~DrAudioFileReader() {}

	// little helper
	static void writeWavFile(const std::string &path, std::vector<float> &data, int numChannels, int sampleRate);

protected:
private:
	std::unique_ptr<DrAudioFileReader> impl;
};

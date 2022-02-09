#include <vector>
struct SpeexResamplerState_; 
typedef struct SpeexResamplerState_ SpeexResamplerState;

class Resampler {
public:
	SpeexResamplerState *resampler = nullptr;
	int numChannels = 0;
	double ratio = 1;

	bool init(int numChannels, int inSampleRate, int outSampleRate, int quality = 9);
		
	
	
	void process(const std::vector<float> &ins, std::vector<float> &outs);
	
	
	virtual ~Resampler();
};


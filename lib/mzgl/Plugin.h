//
//  EffectPlugin.h
//  Koala FX
//
//  Created by Marek Bereza on 22/10/2019.
//  Copyright © 2019 Marek Bereza. All rights reserved.
//

#pragma once
#include <string>
#include <vector>
#include <functional>
#include <map>
#include <atomic>

#include "util.h"
#include "util/log.h"
#include "FloatBuffer.h"
#include "PluginParameter.h"
#include "Midi.h"

struct MidiMessage;


class Serializable {
public:
	virtual std::string getIdentifier() = 0; // { return "koalafx"; }
	virtual void serialize(std::vector<uint8_t> &outData) = 0;
	virtual void deserialize(const std::vector<uint8_t> &outData)  = 0;
	
	
	// this is for if you need to serialize a lot of data
	virtual void serializeByNSDictionary(const void *nsdict) {}
	virtual void deserializeByNSDictionary(const void *nsdict) {}
	virtual bool wantsToSerializeWithNSDictionary() { return false; }
	
	virtual void loadFromFile(std::string path) {
		std::vector<uint8_t> data;
		readFile(path, data);
		deserialize(data);
	}
};


/**
 * Base class for yer presetz
 */
class EffectPreset {
public:
	std::string name;
	std::vector<uint8_t> data;
	EffectPreset(std::string name) : name(name) {}
};


#include "filesystem.h"


// presets are always files, their names are the file name
class PresetManager {
public:
	Serializable *plugin;
	PresetManager(Serializable *plugin) {
		this->plugin = plugin;
		loadFactoryPresets();
	}
	
	void loadFactoryPresets() {
		fs::path presetPath = dataPath("factory-presets");
		try {
			if(!fs::exists(presetPath)) {
				Log::d() << "No factory presets to load";
				return;
			}
			for(auto &p : fs::directory_iterator(presetPath)) {
				if(p.path().extension()==getExt()) {
					factoryPresets.push_back(p.path().stem().string());
				}
			}
		
		} catch(const fs::filesystem_error &err) {
			Log::e() << "fs error loading factory presets " << err.what();
		}
		std::sort(factoryPresets.begin(), factoryPresets.end());
		
	}
	
	std::string getExt() { return "."+plugin->getIdentifier()+"preset"; }
	
	std::vector<std::string> factoryPresets;
	
	std::string currPresetName;
	
	
	
	virtual std::vector<std::string> getFactoryPresetNames() {
		return factoryPresets;
	}
	
	virtual void loadUserPreset(int which) {
		std::string presetName = getUserPresetNames()[which];
		std::string presetPath = getUserPresetDir() + "/" + presetName + getExt();
		plugin->loadFromFile(presetPath);
		currPresetName = presetName;
    }
    
	virtual void loadFactoryPreset(int which) {
		std::string presetPath = dataPath("factory-presets/" + factoryPresets[which] + getExt());
		Log::d() << "preset path " << presetPath;
		plugin->loadFromFile(presetPath);
		currPresetName = factoryPresets[which];
	}
	
	virtual std::vector<std::string> getUserPresetNames() {
		createUserPresetDirIfNotExist();
		
		std::vector<std::string> presetNames;
		
		for(auto &p : fs::directory_iterator( getUserPresetDir())) {
			if(p.path().extension()==getExt()) {
				presetNames.push_back(p.path().stem().string());
			}
		}
		std::sort(presetNames.begin(), presetNames.end());
		
		return presetNames;
	}

	size_t getNumPresets() {
		return factoryPresets.size();
	}

	std::string getUserPresetDir() {
		return docsPath(plugin->getIdentifier() + "/presets");
	}
	
	void createUserPresetDirIfNotExist() {
		std::string path = getUserPresetDir();
		if(!fs::exists(path)) {
			fs::create_directories(path);
		}
	}
	
	void deleteUserPreset(int which) {
		std::string presetToDelete = getUserPresetDir() + "/" + getUserPresetNames()[which] + getExt();
		fs::remove(presetToDelete);
	}
	
	void saveUserPreset(const std::string &name) {
		createUserPresetDirIfNotExist();
		std::string path = getUserPresetDir() + "/" + name + getExt();
		std::vector<uint8_t> data;
		plugin->serialize(data);
		Log::d() << "Writing preset to " << path;
		writeFile(path, data);
	}
	
	std::function<std::vector<std::string>()> getUserPresetNamesCallback = []() -> std::vector<std::string> {return std::vector<std::string>();};

	virtual std::string getCurrentPresetName() { return currPresetName; }
};


struct PluginMidiOutMessage {
	MidiMessage msg;
	int delay; // in samples
	int outputNo;
	PluginMidiOutMessage(int outputNo, const MidiMessage &m, int delay) : outputNo(outputNo), msg(m), delay(delay) {}
};

class PluginEditor;
class Plugin : public Serializable {
public:
	
	Plugin() {
		midiOutMessages.reserve(100);
	}
	
	virtual ~Plugin() {}
	
	friend class PluginEditor;
	
	// allocate memory here
	virtual void init(int numInputs, int numOutputs) {}
	
	// process audio here, no allocating
    // the buffers are arrays of interleaved (for stereo) bus buffers
	virtual void process(FloatBuffer *ins, FloatBuffer *outs, int channelsPerBus) {}
	
    virtual int getNumOutputBusses() { return 1; }
    
	virtual int getNumMidiOuts() {return 0;}
	virtual std::string getMidiOutName(int index) { return ""; }
	
	// MUST HAPPEN ON AUDIO THREAD! (i.e. within process call
	void sendMidiMessage(int outputNo, const MidiMessage &m, int delay) {
		midiOutMessages.emplace_back(outputNo, m, delay);
	};
	
	
	// midi events will come in on audio thread
	virtual void midiReceivedAtTime(const MidiMessage &m, uint32_t delay) {}
	
	size_t getNumParams() {
		return params.size();
	}
	
	size_t getNumPresets() {
		return getPresetManager()->getNumPresets();
	}
	std::shared_ptr<PluginParameter> getParam(unsigned int i) {
		return params[i];
	}
	
	bool isInstrument() { return pluginIsInstrument; }
	void hostUpdatedParameter(unsigned int i, float val) {
		if(i>=params.size() || i < 0) return;
		if(!params[i]->isIgnoringAutomation()) {
			params[i]->set(val);
			if(paramChanged) paramChanged(i, val);
		}
    }
    
	std::function<void(unsigned int, float)> sendUpdatedParameterToHost;
	
    void updateHostParameter(unsigned int i, float val) {
		params[i]->set(val);
		if(sendUpdatedParameterToHost) {
			sendUpdatedParameterToHost(i, val);
		} else {
			Log::d() << "error sendUpdatedParameterToHost not implemented";
		}
    }
	
	std::function<bool()> isRunning = []() { return true; };
	std::shared_ptr<PresetManager> getPresetManager() {
		if(presetManager==nullptr) {
			presetManager = std::make_shared<PresetManager>(this);
		}
		return presetManager;
	}
	
	void setSampleRate(double sampleRate) {
		if(this->sampleRate!=sampleRate) {
			this->sampleRate = sampleRate;
			sampleRateChanged();
		}
	}
	double getSampleRate() {
		return sampleRate;
	}

	virtual void sampleRateChanged() {}
	
	double bpm = 120.0;
	double beatPosition = 0.0;
	
	virtual void hostIsPlayingChanged() {}
	void setHostIsPlaying(bool b) {
		if(b!=hostIsPlaying) {
			hostIsPlaying = b;
			hostIsPlayingChanged();
		}
	}
	
	bool getHostIsPlaying() {
		return hostIsPlaying;
	}
	
	
	// this is so far, just for AUv3 on mac to signify that it's running
	bool hasStarted = false;
	
	bool pluginIsInstrument = false;
	void setIsInstrument(bool v) { pluginIsInstrument = v;}
	
	std::vector<PluginMidiOutMessage> midiOutMessages;
	
protected:
	bool hostIsPlaying = false;
	std::shared_ptr<PresetManager> presetManager;

	// generic float slider
	void addFloatParameter(std::string name, float defaultValue, float from = 0, float to = 1, std::string unit = "") {
		params.push_back(std::make_shared<PluginParameter>(name, defaultValue, from, to, unit));
	}
	
	void addIntParameter(std::string name, int defaultValue, int from = 0, int to = 1, std::string unit = "") {
		params.push_back(std::make_shared<PluginParameter>(name, defaultValue, from, to, unit));
	}
	
	// indexed
	void addIndexedParameter(std::string name, int defaultValue, const std::vector<std::string> &options) {
		params.push_back(std::make_shared<PluginParameter>(name, defaultValue, options));
	}
	
	std::string getParameterName(int i) {
		return params[i]->name;
	}	

	std::vector<std::shared_ptr<PluginParameter>> params;
	std::function<void(unsigned int, float)> paramChanged;
private:
	double sampleRate = 48000.0;
};

std::shared_ptr<Plugin> instantiatePlugin();



//
//  Pattern.h
//  This is a pattern of notes - a sequence if you will.
//
//  Created by Marek Bereza on 28/09/2015.
//
//

#pragma once
#include "Note.h"
//#include "ofxMidi.h"
#include "json.hpp"
#include "filesystem.h"
//#ifdef _WIN32
//#include "mingw.mutex.h"
//#define MUTEX mingw_stdthread::mutex
//#else
#include <mutex>
//#define MUTEX std::mutex
//#endif
//#include "utils.h"
using namespace std;
#ifndef     MIDI_MIN_BEND
#define MIDI_NOTE_OFF			0x80
#define MIDI_NOTE_ON			0x90
#endif


#define SAMPLERATE 44100

// time type, in pulses (as defined by ppqn - pulses per quarter-note)
typedef int TimeOffset;
typedef std::multimap<TimeOffset,Note>::iterator PatternNote;
#define METRO_TICK_MAJOR -10
#define METRO_TICK_MINOR -11

using namespace nlohmann;

class NoteCommand {
public:

	int status; // midi note on or off

	int delay; // in samples from the frame

	int pitch;

	int velocity;

	NoteCommand(int status = MIDI_NOTE_ON, int delay = 0, int pitch = 0, int velocity = 0) {
		this->status = status;
		this->delay = delay;
		this->pitch = pitch;
		this->velocity = velocity;
	}


    string statusString() {
        switch(status) {
            case MIDI_NOTE_ON: return "midi note on";
            case MIDI_NOTE_OFF: return "midi note off";
            case METRO_TICK_MAJOR: return "metro tick major";
            case METRO_TICK_MINOR: return "metro tick minor";

            default: return "unknown";
        }
    }
    string toString() {
        return statusString() + "     delay: " + to_string(delay) + "   pitch: " + to_string(pitch) + "   velocity: " + to_string(velocity);
    }
};


class NotePattern {

public:
	int numBars;
	int beatsPerBar;
	int ppqn;
	bool metroOn;
	multimap<TimeOffset,Note> notes;

	static mutex patternMutex;



	bool operator==(const NotePattern &other) const;

    // checks that all the notes in this pattern are also in the
    // other note pattern (there may be extra notes in the other pattern)
    // so to check for equality you have to test both ways
	bool containsAllTheSameNotes(const NotePattern &other) const;


	bool contains(const TimeOffset &t, Note &n) const;

    bool operator!=(const NotePattern &other) const {
        return !((*this)==other);
    }

	void save(string path);

	nlohmann::json createNoteJson(TimeOffset to, const Note &n);

	void clear() {
		lock();
		notes.clear();
		unlock();
	}


	void quantize();

	void load(string path);


	NotePattern();


	void lock() {
		patternMutex.lock();
	}

	void unlock() {
		patternMutex.unlock();
	}

	const PatternNote &insertNote(Note note, TimeOffset pos);

	TimeOffset generatePosition(int barNo, int beatNo, int ppqnOffset = 0) {
		return (barNo * beatsPerBar + beatNo) * ppqn + ppqnOffset;
	}
	PatternNote beginIt;
	const PatternNote &begin() {
		beginIt = notes.begin();
		return beginIt;
	}

	PatternNote endIt;
	const PatternNote &end() {
		endIt = notes.end();
		return endIt;
	}


	PatternNote findIt;
	const PatternNote &find(int pitch, TimeOffset t);


	void erase(const PatternNote &note) {
		notes.erase(note);
	}

    int getTimeOfBeatInSamples();

	int getPatternLengthInSamples() {
		return getTimeOfBeatInSamples() * beatsPerBar * numBars;
	}

	int getPatternLength() {
		return beatsPerBar * numBars * ppqn;
	}

	void doubleUp();

	int timeOffsetToSamples(TimeOffset t) {
		return getPatternLengthInSamples() * (float)(t / (float)(numBars * beatsPerBar * ppqn));
	}

	TimeOffset samplesToTimeOffset(int s) {
		//return (numBars * beatsPerBar * ppqn * s) / (float)getPatternLengthInSamples();
		return (ppqn * s) / (float)(getTimeOfBeatInSamples());
	}


	void doMetro(int start, int finish, vector<NoteCommand> &noteCommands);

	// must take care of wrapping the finish value
	// TODO: I think the missing notes are due to rounding errors in time offset to samples
	void getAllNoteCommands(int start, int finish, vector<NoteCommand> &noteCommands);

	TimeOffset quantizeTiming(TimeOffset t, int division);
};






//
//  NotePattern.cpp
//  emptyExample
//
//  Created by Marek Bereza on 30/04/2017.
//
//

#include "NotePattern.h"
#include <iomanip>

///#include "Song.h"
mutex NotePattern::patternMutex;

int NotePattern::getTimeOfBeatInSamples() {
	float secondsPerBeat = 60.f/ 120.f;//Song::getInstance()->bpm;
	return secondsPerBeat * SAMPLERATE;
}

bool NotePattern::operator==(const NotePattern &other) const {
	if(!(numBars==other.numBars && beatsPerBar==other.beatsPerBar && ppqn==other.ppqn)) {
		return false;
	}
	if(notes.size()!=other.notes.size()) {
		return false;
	}
	return containsAllTheSameNotes(other)
	&& ((NotePattern&)other).containsAllTheSameNotes(*this);
}


// checks that all the notes in this pattern are also in the
// other note pattern (there may be extra notes in the other pattern)
// so to check for equality you have to test both ways
bool NotePattern::containsAllTheSameNotes(const NotePattern &other) const {
	for(auto it = notes.begin(); it != notes.end(); it++) {
		TimeOffset t = (*it).first;
		Note n = (*it).second;
		if(!other.contains(t, n)) {
			return false;
		}
	}
	return true;
}


bool NotePattern::contains(const TimeOffset &t, Note &n) const {
	for(auto it = notes.find(t); it != notes.end(); it++) {
		if(n==(*it).second) return true;
	}
	return false;
}


void NotePattern::save(string path) {


	json j;
	json noteObj;
	for(auto it = begin(); it != end(); it++) {
		noteObj.push_back(createNoteJson((*it).first, (*it).second));
	}
	j["pattern"] = {{"numBars", numBars}, {"beatsPerBar", beatsPerBar}, {"ppqn", ppqn}, {"notes", noteObj}};
	fs::ofstream o(fs::u8path(path));
	o << std::setw(4) << j << std::endl;
}

nlohmann::json NotePattern::createNoteJson(TimeOffset to, const Note &n) {
	json j;
	j["timeOffset"] = to;
	j["pitch"] = n.pitch;
	j["vel"] = n.vel;
	j["length"] = n.length;

	return j;
}

TimeOffset NotePattern::quantizeTiming(TimeOffset t, int division) {

	// this is the quantize factor for TimeOffset
	int ppDiv = (ppqn * 4) / division;
	double tt = round(t / (double)ppDiv);
	return (TimeOffset) round(tt * ppDiv);
}
void NotePattern::quantize() {

	multimap<TimeOffset,Note> quantized;

	for(auto &note: notes) {
		quantized.insert(make_pair(quantizeTiming(note.first, 16), note.second));
	}
	notes = quantized;

}

void NotePattern::load(string path) {

	notes.clear();

	fs::ifstream i(fs::u8path(path));
	json j;
	i >> j;

	j = j["pattern"];

	numBars		= j["numBars"];
	beatsPerBar = j["beatsPerBar"];

	j = j["notes"];

	for(auto &note : j) {
		Note n;
		TimeOffset t	= note["timeOffset"];
		n.pitch			= note["pitch"];
		n.vel			= note["vel"];
		n.length		= note["length"];

		insertNote(n, t);
	}
}



NotePattern::NotePattern() {
	numBars = 1;
	beatsPerBar = 4;
	metroOn = true;
}




const PatternNote &NotePattern::insertNote(Note note, TimeOffset pos) {
	// not sure if the below line is safe, returning an iterator - don't know if it's still valid down the line
	// maybe need to make some sort of object representing this iterator that manages whether it's still valid.
	// also, using the find function, we're not guaranteed that it's the same note that's just been inserted,
	// but if we send back the itereator from the insert function, it definitely crashes (and XCode says that
	// the iterator is temporary.
	printf("%d\n", note.length);
	notes.insert(make_pair(pos, note));
	return find(note.pitch, pos);
}





const PatternNote &NotePattern::find(int pitch, TimeOffset t) {

	int patternLength = getPatternLength();

	for(findIt = begin(); findIt != end(); findIt++) {
		Note &n = (*findIt).second;
		TimeOffset start = (*findIt).first;
		if(n.pitch==pitch) {


			if(t>=start && t < start + n.length) { // normal notes
				return findIt;
			} else if(start + n.length > patternLength // if length goes over the end and wraps
					  &&
					  (t >= start
					  ||
					   t < start + n.length - patternLength

					   )

					  ) { // wrapping notes
				return findIt;

			}
		}
	}
	return end();
}




void NotePattern::doubleUp() {
	int oldNumBars = numBars;
	numBars *= 2;

	vector<pair<TimeOffset, Note> > newNotes;
	for(auto it = begin(); it != end(); it++) {
		newNotes.push_back(make_pair((*it).first, (*it).second));
	}

	for(int i = 0; i < newNotes.size(); i++) {

		// offset the note by the length of the old measure
		newNotes[i].first += oldNumBars * beatsPerBar * ppqn;
		insertNote(newNotes[i].second, newNotes[i].first);
	}
}





void NotePattern::doMetro(int start, int finish, vector<NoteCommand> &noteCommands) {
	TimeOffset a = samplesToTimeOffset(start);
	TimeOffset b = samplesToTimeOffset(finish);

	if(a>b) { // fix wrapping
		a -= numBars * beatsPerBar * ppqn;
		//printf("%d -> %d\n", a, b);

	}

	for(TimeOffset i = a; i < b; i++) {

		if(i%(beatsPerBar*ppqn)==0) { // bar beep
			int d = (timeOffsetToSamples(i) - start);
			if(d<0) d += getPatternLengthInSamples();
			noteCommands.push_back(NoteCommand(METRO_TICK_MAJOR, d));
		} else if(i%ppqn==0) { // beat beep
			noteCommands.push_back(NoteCommand(METRO_TICK_MINOR, timeOffsetToSamples(i) - start));
		}
	}
}

// must take care of wrapping the finish value
// TODO: I think the missing notes are due to rounding errors in time offset to samples
void NotePattern::getAllNoteCommands(int start, int finish, vector<NoteCommand> &noteCommands) {
	// work out if there are any metros
	// bar boundary, beatBoundary numBars beatsPerBar ppqn


	if(metroOn) doMetro(start, finish, noteCommands);



	for(auto it = begin(); it != end(); it++) {

		// if the end of a note is in the bracket, do a note off
		int noteStart = timeOffsetToSamples((*it).first);
		int noteEnd = noteStart + timeOffsetToSamples((*it).second.length);

		// have to check for wrap around stuff

		if(finish>start) {
			if(noteStart >= start && noteStart < finish) {
				noteCommands.push_back(NoteCommand(MIDI_NOTE_ON, noteStart - start, (*it).second.pitch, (*it).second.vel));
			}

			if(noteEnd >= start && noteEnd < finish) {
				noteCommands.push_back(NoteCommand(MIDI_NOTE_OFF, noteEnd - start, (*it).second.pitch));
			}
		} else {
			// this is the wraparound case, where the buffer crosses the pattern loop boundary
			if(noteStart >= start) {
				noteCommands.push_back(NoteCommand(MIDI_NOTE_ON,  noteStart - start, (*it).second.pitch, (*it).second.vel));
			} else if(noteStart < finish) {
				noteCommands.push_back(NoteCommand(MIDI_NOTE_ON,  noteStart - (start - getPatternLengthInSamples()), (*it).second.pitch, (*it).second.vel));
			}


			if(noteEnd >= start) {
				noteCommands.push_back(NoteCommand(MIDI_NOTE_OFF, noteEnd - start, (*it).second.pitch, (*it).second.vel));
			} else if(noteEnd < finish) {
				noteCommands.push_back(NoteCommand(MIDI_NOTE_OFF, noteEnd - (start - getPatternLengthInSamples()), (*it).second.pitch, (*it).second.vel));
			}
		}
	}
}


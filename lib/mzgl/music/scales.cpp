/**
 *  scales.cpp
 *
 *  Created by Marek Bereza on 05/02/2013.
 */

#include <mzgl/music/scales.h>
#include <math.h>
#include <vector>

static std::vector<std::string> noteNames = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

std::string noteNumToName(int note) {
	if (note < 0 || note > 11) return "n/a";
	return noteNames[note];
}

int noteNameToNum(const std::string &noteName) {
	for (int i = 0; i < noteNames.size(); i++) {
		if (noteNames[i] == noteName) return i;
	}
	return -1;
}

// this is for C3 = 60 - which is the ableton convention
std::string midiNoteNumToString(int note) {
	if (note < 0) return "n/a";

	int octave = (note / 12) - 2;
	int index  = note % 12;
	return noteNumToName(index) + std::to_string(octave);
}

float mtof(float f) {
	return (8.17579891564 * exp(.0577622650 * f));
}

float ftom(float f) {
	return (17.3123405046 * log(.12231220585 * f));
}

int getScaled(int pos, int scale) {
	if (scale == CHROMATIC) return pos;
	else if (scale == PENTATONIC) {
		int octave = pos / 5;
		int note   = 0;
		switch (pos % 5) {
			case 0: note = 0; break;
			case 1: note = 3; break;
			case 2: note = 5; break;
			case 3: note = 7; break;
			case 4: note = 10; break;
		}
		return octave * 12 + note;
	}

	else if (scale == MINOR) {
		int octave = pos / 7;
		int note   = 0;
		switch (pos % 7) {
			case 0: note = 0; break;
			case 1: note = 2; break;
			case 2: note = 3; break;
			case 3: note = 5; break;
			case 4: note = 7; break;
			case 5: note = 8; break;
			case 6: note = 11; break;
		}
		return octave * 12 + note;
	}

	else if (scale == MAJOR) {
		int octave = pos / 7;
		int note   = 0;
		switch (pos % 7) {
			case 0: note = 0; break;
			case 1: note = 2; break;
			case 2: note = 4; break;
			case 3: note = 5; break;
			case 4: note = 7; break;
			case 5: note = 9; break;
			case 6: note = 11; break;
		}
		return octave * 12 + note;
	}

	else if (scale == WHOLE) {
		int octave = pos / 6;
		int note   = 0;
		switch (pos % 6) {
			case 0: note = 0; break;
			case 1: note = 2; break;
			case 2: note = 4; break;
			case 3: note = 6; break;
			case 4: note = 8; break;
			case 5: note = 10; break;
		}
		return octave * 12 + note;
	}
#pragma warning not sure what this does
	return pos;
}

float midiNoteToSpeed(int midiNote, int originalNote) {
	return pow(2, (midiNote - originalNote) / 12.f);
}

int qwertyToMidi(int k) {
	switch (k) {
		case 'a': return 0;
		case 'w': return 1;
		case 's': return 2;
		case 'e': return 3;
		case 'd': return 4;
		case 'f': return 5;
		case 't': return 6;
		case 'g': return 7;
		case 'y': return 8;
		case 'h': return 9;
		case 'u': return 10;
		case 'j': return 11;
		case 'k': return 12;
		case 'o': return 13;
		case 'l': return 14;
		case 'p': return 15;
		case ';': return 16;
		case '\'': return 17;

		default: return -1;
	}
}

bool isSharp(int i) {
	switch ((i + 144) % 12) {
		case 1:
		case 3:
		case 6:
		case 8:
		case 10: return true;
		default: return false;
	}
}

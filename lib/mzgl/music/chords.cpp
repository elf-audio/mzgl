#include "chords.h"
//#include <stdio.h>
//#include <algorithm>
#include "scales.h"

enum class MajorMinor { None, Major, Minor };
enum class Extension { None, Perfect, Flat, Sharp };
static std::string tonality(MajorMinor majorMinor) {
	switch (majorMinor) {
		// case MajorMinor::Major: return "Maj";
		case MajorMinor::Minor: return "m";
		default: return "";
	}
}
static std::string seventhStr(MajorMinor majorMinor) {
	switch (majorMinor) {
		case MajorMinor::Major: return "maj7";
		case MajorMinor::Minor: return "7";
		default: return "";
	}
}
static std::string fifthStr(Extension fifth) {
	switch (fifth) {
		case Extension::Flat: return "b5";
		case Extension::Sharp: return "#5";
		default: return "";
	}
}
std::string notesToChordName(const std::vector<int> &_chord) {
	std::vector<int> chord = _chord;
	if (chord.empty()) return "Unknown";
	if (chord.size() < 2) return noteNumToName(chord[0]);
	auto rootNote = chord[0] % 12;

	for (auto &n: chord) {
		n = (n + 12 - rootNote) % 12;
	}

	chord.erase(std::remove(chord.begin(), chord.end(), 0), chord.end());
	chord.erase(std::remove(chord.begin(), chord.end(), 7), chord.end());

	std::sort(chord.begin(), chord.end());

	// remove duplicates
	chord.erase(std::unique(chord.begin(), chord.end()), chord.end());

	std::string note = noteNumToName(rootNote);
	if (chord.size() == 1) {
		if (chord[0] == 4) return note;
		if (chord[0] == 3) return note + "m";
		if (chord[0] == 2) return note + "sus2";
		if (chord[0] == 5) return note + "sus4";
		return noteNumToName(rootNote) + "?";
	}
	if (chord.size() == 2) {
		if (chord[0] == 2) {
			if (chord[1] == 3) return note + "madd9";
			if (chord[1] == 4) return note + "add9";
		} else if (chord[0] == 3) {
			if (chord[1] == 10) return note + "m7";
			if (chord[1] == 11) return note + "mM7";
			if (chord[1] == 9) return note + "m6";
			if (chord[1] == 6) return note + "dim";
		} else if (chord[0] == 4) {
			if (chord[1] == 10) return note + "7";
			if (chord[1] == 11) return note + "maj7";
			if (chord[1] == 9) return note + "6";
			if (chord[1] == 8) return note + "aug";
		}

		return note + "?";
	}

	//	    Cadd
	if (chord.size() == 3) {
		if (chord[0] == 2 && chord[1] == 4 && chord[2] == 9) return note + "6/9";
		if (chord[0] == 2 && chord[1] == 4 && chord[2] == 10) return note + "9";
		if (chord[0] == 2 && chord[1] == 3 && chord[2] == 10) return note + "m9";
		if (chord[0] == 2 && chord[1] == 4 && chord[2] == 11) return note + "maj9";
		if (chord[0] == 3 && chord[1] == 6 && chord[2] == 10) return note + "m7b5";
		if (chord[0] == 4 && chord[1] == 8 && chord[2] == 10) return note + "7#5";
		if (chord[0] == 4 && chord[1] == 6 && chord[2] == 10) return note + "7b5";
		if (chord[0] == 3 && chord[1] == 6 && chord[2] == 9) return note + "dim7";
		if (chord[0] == 4 && chord[1] == 8 && chord[2] == 10) return note + "aug7";
		return noteNumToName(rootNote) + "?";
	}

	if (chord.size() == 4) {
		if (chord[0] == 2) {
			if (chord[1] == 4 && chord[2] == 5 && chord[3] == 10) return note + "11";
			if (chord[1] == 3 && chord[2] == 5 && chord[3] == 10) return note + "m11";
			if (chord[1] == 3 && chord[2] == 5 && chord[3] == 11) return note + "maj11";
			if (chord[1] == 4 && chord[2] == 9 && chord[3] == 11) return note + "maj13";
		}
		return noteNumToName(rootNote) + "?";
	}

	if (chord.size() == 5) {
		if (chord[0] == 2 && chord[1] == 4 && chord[2] == 5 && chord[3] == 9 && chord[4] == 10) return note + "13";
		if (chord[0] == 2 && chord[1] == 3 && chord[2] == 5 && chord[3] == 9 && chord[4] == 10)
			return note + "m13";
	}
	return noteNumToName(rootNote) + "?"; //tonality(majorMinor) + seventhStr(seventh) + fifthStr(fifth);
}

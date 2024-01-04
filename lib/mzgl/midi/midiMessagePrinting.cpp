//
// Created by Marek Bereza on 04/01/2024.
//

#include "midiMessagePrinting.h"

std::ostream &operator<<(std::ostream &os, const MidiMessage &msg) {
	os << "MidiMessage: ["
	   << "]";
	return os;
}
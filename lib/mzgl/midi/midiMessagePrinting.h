//
// Created by Marek Bereza on 04/01/2024.
//

#pragma once
#include <mzgl/midi/MidiMessage.h>
#include <string>
#include <sstream>

std::ostream &operator<<(std::ostream &os, const MidiMessage &msg);

//
//  Midi.cpp
//  MZGL
//
//  Created by Marek Bereza on 06/04/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#include "Midi.h"
void MidiInCallback(double deltatime, std::vector<unsigned char> *message, void *userData) {
	((MidiIn *) userData)->callback(deltatime, message);
}

//
//  Midi.cpp
//  MZGL
//
//  Created by Marek Bereza on 06/04/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#include <mzgl/midi/Midi.h>
#ifdef __APPLE__
#	include <TargetConditionals.h>
#endif

#if TARGET_OS_MAC && !TARGET_OS_IOS
void MidiInCallback(double deltatime, std::vector<unsigned char> *message, void *userData) {
	((MidiIn *) userData)->callback(deltatime, message);
}
#endif

//
//  RtMidiExtras.h
//  MZGL
//
//  Created by Marek Bereza on 23/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#pragma once

/*
 
 MODIFY RTMIDI TO HAVE THIS FILE INCLUDED IN RtMidi.h
 AND COMMENT OUT
 
 		#include <CoreAudio/HostTime.h>
 		#include <CoreServices/CoreServices.h>
 
 FROM RtMidi.cpp
 
 */
#define __MACOSX_CORE__

#include <TargetConditionals.h>
#if TARGET_OS_IOS
#	include "CAHostTimeBase.h"
//#define AudioGetCurrentHostTime() (0)
//#define AudioConvertHostTimeToNanos(A) (A)
//#define EndianS32_BtoN(A) (A)
#	include "Endian.h"

#else

#	include <CoreAudio/HostTime.h>
#	include <CoreServices/CoreServices.h>
#endif

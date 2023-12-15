//
//  appleMidiUtils.cpp
//  mzgl
//
//  Created by Marek Bereza on 14/11/2021.
//  Copyright © 2021 Marek Bereza. All rights reserved.
//

#include "appleMidiUtils.h"
#include <dispatch/dispatch.h>

static double __hostTicksToNanoSeconds = 0.0;
static double __hostTicksToSeconds	   = 0.0;
static double __secondsToHostTicks	   = 0.0;
#include <mach/mach_time.h>

static void SEMIDIInit(void) {
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
	  mach_timebase_info_data_t tinfo;
	  mach_timebase_info(&tinfo);
	  __hostTicksToSeconds	   = ((double) tinfo.numer / tinfo.denom) * 1.0e-9;
	  __hostTicksToNanoSeconds = (tinfo.numer / (double) tinfo.denom);
	  __secondsToHostTicks	   = 1.0 / __hostTicksToSeconds;
	});
}

uint64_t hostTicksToSeconds(uint64_t ticks) {
	if (!__hostTicksToSeconds) SEMIDIInit();
	return ticks * __hostTicksToSeconds;
}

uint64_t hostTicksToNanoSeconds(uint64_t ticks) {
	if (!__hostTicksToNanoSeconds) SEMIDIInit();
	return ticks * __hostTicksToNanoSeconds;
}

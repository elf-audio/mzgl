#pragma once

// Diagnostic tool: tracks every live sg_buffer with the backtrace of its
// allocation site, so sokol buffer-pool slot usage can be attributed to the
// code that created it. touch() marks a buffer as bound-for-draw so dump()
// can report how recently each buffer was actually used.
//
// Works on macOS/iOS, Linux (glibc) and Windows (DbgHelp); compiles to no-ops
// on platforms without a backtrace facility (Android, web).

#include "sokol_gfx.h"
#include <string>

namespace SokolBufferTracker {

	// Tracking is OFF by default - track/untrack/touch are a single branch when
	// disabled. Enable at startup (before buffers are created, or the report
	// will be partial) when diagnosing buffer-slot usage; capturing a backtrace
	// per buffer creation and a map lookup per bind is not free.
	void setTrackingEnabled(bool enabled);
	bool trackingEnabled();

	// Record a freshly created buffer with the current call stack.
	void track(sg_buffer b, int size, bool pooled);

	// Forget a destroyed buffer.
	void untrack(sg_buffer b);

	// Mark a buffer as bound for a draw this frame.
	void touch(sg_buffer b);

	// Symbolicated report of all live buffers grouped by allocation stack,
	// biggest group first.
	std::string dump();

} // namespace SokolBufferTracker

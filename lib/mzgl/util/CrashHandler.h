// Generic crash-reporting hook. mzgl sets up the unhandled-exception filter
// (on Windows) and the WinMain catch path, and delegates the actual handling
// — log writing, dialogs, uploads — to an app-registered callback.
//
// Apps that want crash handling register a callback at static-init time:
//
//     static struct R { R() { mzgl::setCrashHandler(myHandler); } } r;
//
// Apps that don't register a handler get the default behaviour: nothing
// (other than the OS terminating the process). mzgl itself never knows about
// app-specific secrets, URLs, or paths.

#pragma once

#include <functional>
#include <string>

#ifdef _WIN32
#	include <windows.h>
#endif

namespace mzgl {

enum class CrashKind {
	SEH,			  // unhandled SEH (Windows access violation, etc.)
	CppException,	  // catch (const std::exception&) in WinMain
	UnknownException, // catch (...) in WinMain
};

struct CrashInfo {
	CrashKind kind;
	std::string description; // SEH name, e.what(), or "unknown"

#ifdef _WIN32
	DWORD sehCode					 = 0;
	void *exceptionAddress			 = nullptr;
	EXCEPTION_POINTERS *sehPointers	 = nullptr; // valid only inside the SEH filter
#endif
};

using CrashHandlerFn = std::function<void(const CrashInfo &)>;

// Register the handler. Replaces any previous one. Call from a static
// initializer or early app startup. Thread-safe to set but the handler
// itself is invoked from the SEH filter / WinMain catch — keep it minimal.
void setCrashHandler(CrashHandlerFn fn);

// Returns the currently registered handler, or an empty function if none.
const CrashHandlerFn &getCrashHandler();

#ifdef _WIN32
// Maps a SEH ExceptionCode to a short name ("ACCESS_VIOLATION", etc.).
std::string sehCodeToString(DWORD code);

// Walks the current call stack via dbghelp and returns it as a printable
// multi-line string. skipFrames lets you skip wrapper frames at the top.
std::string captureStackTrace(int skipFrames = 0);
#endif

} // namespace mzgl

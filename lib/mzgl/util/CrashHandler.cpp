#include "CrashHandler.h"

#ifdef _WIN32
#	include <dbghelp.h>
#	include <cstdio>
#	pragma comment(lib, "dbghelp.lib")
#endif

namespace mzgl {

namespace {
// Function-local static avoids the static-init-order fiasco — apps register
// their handler from another TU's static initializer, and we don't know which
// TU runs first.
CrashHandlerFn &handlerStorage() {
	static CrashHandlerFn fn;
	return fn;
}
} // namespace

void setCrashHandler(CrashHandlerFn fn) {
	handlerStorage() = std::move(fn);
}

const CrashHandlerFn &getCrashHandler() {
	return handlerStorage();
}

#ifdef _WIN32
std::string sehCodeToString(DWORD code) {
	switch (code) {
		case EXCEPTION_ACCESS_VIOLATION: return "ACCESS_VIOLATION";
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "ARRAY_BOUNDS_EXCEEDED";
		case EXCEPTION_STACK_OVERFLOW: return "STACK_OVERFLOW";
		case EXCEPTION_ILLEGAL_INSTRUCTION: return "ILLEGAL_INSTRUCTION";
		case EXCEPTION_INT_DIVIDE_BY_ZERO: return "INT_DIVIDE_BY_ZERO";
		case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "FLT_DIVIDE_BY_ZERO";
		case EXCEPTION_IN_PAGE_ERROR: return "IN_PAGE_ERROR";
		default: return "UNKNOWN";
	}
}

std::string captureStackTrace(int skipFrames) {
	HANDLE process = GetCurrentProcess();
	SymInitialize(process, nullptr, TRUE);

	void *stack[64];
	WORD frames = CaptureStackBackTrace(static_cast<DWORD>(skipFrames + 1), 64, stack, nullptr);

	char symbolBuf[sizeof(SYMBOL_INFO) + 256];
	SYMBOL_INFO *symbol	 = reinterpret_cast<SYMBOL_INFO *>(symbolBuf);
	symbol->MaxNameLen	 = 255;
	symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

	IMAGEHLP_LINE64 line;
	line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

	std::string out;
	char buf[512];
	snprintf(buf, sizeof(buf), "  Stack trace (%u frames):\n", frames);
	out += buf;
	for (WORD i = 0; i < frames; i++) {
		DWORD64 address = reinterpret_cast<DWORD64>(stack[i]);
		if (SymFromAddr(process, address, nullptr, symbol)) {
			DWORD displacement = 0;
			if (SymGetLineFromAddr64(process, address, &displacement, &line)) {
				snprintf(buf, sizeof(buf), "    [%2d] %s (%s:%lu)\n", i, symbol->Name, line.FileName,
						 line.LineNumber);
			} else {
				snprintf(buf, sizeof(buf), "    [%2d] %s (0x%llX)\n", i, symbol->Name, address);
			}
		} else {
			snprintf(buf, sizeof(buf), "    [%2d] 0x%llX\n", i, address);
		}
		out += buf;
	}
	SymCleanup(process);
	return out;
}
#endif

} // namespace mzgl

#include "GLFWAppRunner.h"
#include <string>

#ifdef _WIN32
#	include <windows.h>
#	include <cstdio>
#	include "mzgl/util/CrashHandler.h"

inline void AttachOrAllocConsoleForLogs(bool alloc_if_needed = false) {
	if (AttachConsole(ATTACH_PARENT_PROCESS) || (alloc_if_needed && AllocConsole())) {
		FILE *f;
		freopen_s(&f, "CONOUT$", "w", stdout);
		freopen_s(&f, "CONOUT$", "w", stderr);
		freopen_s(&f, "CONIN$", "r", stdin);
		setvbuf(stdout, nullptr, _IONBF, 0);
		setvbuf(stderr, nullptr, _IONBF, 0);
	}
}

static void dispatchCrash(const mzgl::CrashInfo &info) {
	const auto &handler = mzgl::getCrashHandler();
	if (handler) handler(info);
}

static LONG WINAPI unhandledExceptionFilter(EXCEPTION_POINTERS *info) {
	mzgl::CrashInfo ci {};
	ci.kind				= mzgl::CrashKind::SEH;
	ci.sehCode			= info->ExceptionRecord->ExceptionCode;
	ci.exceptionAddress = info->ExceptionRecord->ExceptionAddress;
	ci.sehPointers		= info;
	ci.description		= mzgl::sehCodeToString(ci.sehCode);
	dispatchCrash(ci);
	return EXCEPTION_EXECUTE_HANDLER;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	SetUnhandledExceptionFilter(unhandledExceptionFilter);
	int argc = 1;
	// Attach to the terminal if one exists. No console is created when launched from Explorer.
	AttachOrAllocConsoleForLogs(false);

	char arg0[] = "mzgl";
	char *argv[1];
	argv[0] = arg0;

#else
int main(int argc, char *argv[]) {
#endif

	try {
		GLFWAppRunner app;
		app.run(argc, argv);
#ifdef _WIN32
	} catch (const std::exception &e) {
		mzgl::CrashInfo ci {};
		ci.kind		   = mzgl::CrashKind::CppException;
		ci.description = e.what();
		dispatchCrash(ci);
	} catch (...) {
		mzgl::CrashInfo ci {};
		ci.kind		   = mzgl::CrashKind::UnknownException;
		ci.description = "unknown";
		dispatchCrash(ci);
	}
#else
	} catch (const std::exception &e) {
		fprintf(stderr, "Unhandled exception: %s\n", e.what());
	} catch (...) {
		fprintf(stderr, "Unhandled exception (unknown)\n");
	}
#endif
	return 0;
}

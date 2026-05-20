#include "GLFWAppRunner.h"
#include <string>
#include <vector>

#ifdef _WIN32
#	include <windows.h>
#	include <shellapi.h>
#	include <cstdio>
#	include <fcntl.h>
#	include <io.h>
#	include "mzgl/util/CrashHandler.h"

// When the parent process passes a pipe or file in one of the standard
// handles (e.g. node's child_process.spawn with stdio:'pipe', or `Koala > log.txt`
// from cmd), MSVCRT does not automatically wire std::cout/std::cerr to it
// for a /SUBSYSTEM:WINDOWS app. Reattach manually so test harnesses can read
// the "STARTED <port>" handshake and other log output.
inline bool ReattachStdStreamToInheritedHandle(DWORD whichHandle, FILE *targetStream) {
	HANDLE h = GetStdHandle(whichHandle);
	if (h == NULL || h == INVALID_HANDLE_VALUE) return false;
	const DWORD type = GetFileType(h);
	// FILE_TYPE_CHAR means a console - the AttachConsole path below handles that.
	// We only care about pipes (spawned from another process) and disk files (redirected).
	if (type != FILE_TYPE_PIPE && type != FILE_TYPE_DISK) return false;
	int fd = _open_osfhandle(reinterpret_cast<intptr_t>(h), _O_TEXT);
	if (fd < 0) return false;
	const char *mode = (whichHandle == STD_INPUT_HANDLE) ? "r" : "w";
	FILE *f			 = _fdopen(fd, mode);
	if (!f) return false;
	*targetStream = *f;
	setvbuf(targetStream, nullptr, _IONBF, 0);
	return true;
}

inline void AttachOrAllocConsoleForLogs(bool alloc_if_needed = false) {
	// First, if a parent has handed us pipes/files via the standard handles
	// (no console involved), wire stdio to those and we're done.
	const bool outRedirected = ReattachStdStreamToInheritedHandle(STD_OUTPUT_HANDLE, stdout);
	const bool errRedirected = ReattachStdStreamToInheritedHandle(STD_ERROR_HANDLE, stderr);
	if (outRedirected && errRedirected) return;

	if (AttachConsole(ATTACH_PARENT_PROCESS) || (alloc_if_needed && AllocConsole())) {
		FILE *f;
		if (!outRedirected) freopen_s(&f, "CONOUT$", "w", stdout);
		if (!errRedirected) freopen_s(&f, "CONOUT$", "w", stderr);
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
	// Attach to the terminal if one exists. No console is created when launched from Explorer.
	AttachOrAllocConsoleForLogs(false);

	// Parse the OS command line into argv (UTF-8) so flags like
	// --start-test-server reach hasCommandLineFlag(). lpCmdLine from
	// WinMain omits argv[0] and may be mangled for unicode args - use
	// CommandLineToArgvW(GetCommandLineW(), ...) which gives us argv[0] too.
	int wargc			   = 0;
	LPWSTR *wargv		   = CommandLineToArgvW(GetCommandLineW(), &wargc);
	std::vector<std::string> argStorage;
	std::vector<char *> argvVec;
	argStorage.reserve(wargc > 0 ? wargc : 1);
	argvVec.reserve((wargc > 0 ? wargc : 1) + 1);
	for (int i = 0; i < wargc; i++) {
		const int n = WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, nullptr, 0, nullptr, nullptr);
		std::string s((n > 0 ? n - 1 : 0), '\0');
		if (n > 0) WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, s.data(), n, nullptr, nullptr);
		argStorage.push_back(std::move(s));
	}
	if (wargv != nullptr) LocalFree(wargv);
	if (argStorage.empty()) argStorage.emplace_back("mzgl");
	for (auto &a : argStorage) argvVec.push_back(a.data());
	argvVec.push_back(nullptr); // POSIX-style sentinel
	int argc	= static_cast<int>(argStorage.size());
	char **argv = argvVec.data();

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

#include "GLFWAppRunner.h"
#include <fstream>
#include <string>
#include <ctime>
#include <cstdio>

#ifdef _WIN32
#	include <windows.h>
#	include <shlobj.h>

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
#endif

static std::string getCrashLogPath() {
#ifdef _WIN32
	wchar_t *wpath = nullptr;
	if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_AppDataDocuments, KF_FLAG_CREATE, nullptr, &wpath))) {
		char path[MAX_PATH];
		WideCharToMultiByte(CP_UTF8, 0, wpath, -1, path, MAX_PATH, nullptr, nullptr);
		CoTaskMemFree(wpath);
		std::string dir = std::string(path) + "\\Koala";
		CreateDirectoryA(dir.c_str(), nullptr);
		return dir + "\\crash.log";
	}
#endif
	return "crash.log";
}

static void writeCrashLog(const std::string &msg) {
	try {
		std::ofstream f(getCrashLogPath(), std::ios::app);
		std::time_t t = std::time(nullptr);
		char timeBuf[64];
		std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
		f << "[" << timeBuf << "] " << msg << "\n";
	} catch (...) {
	}
}

static void showCrashDialog(const std::string &title, const std::string &message) {
#ifdef _WIN32
	MessageBoxA(nullptr, message.c_str(), title.c_str(), MB_OK | MB_ICONERROR);
#elif defined(__linux__)
	std::string cmd = "zenity --error --title=\"" + title + "\" --text=\"" + message + "\" 2>/dev/null";
	if (system(cmd.c_str()) != 0) {
		fprintf(stderr, "%s: %s\n", title.c_str(), message.c_str());
	}
#else
	fprintf(stderr, "%s: %s\n", title.c_str(), message.c_str());
#endif
}

#ifdef _WIN32
static LONG WINAPI unhandledExceptionFilter(EXCEPTION_POINTERS *info) {
	std::string msg = "Unhandled exception, code: 0x";
	char buf[32];
	snprintf(buf, sizeof(buf), "%08lX", info->ExceptionRecord->ExceptionCode);
	msg += buf;
	writeCrashLog(msg);
	showCrashDialog("Koala - Crash",
					"Koala crashed unexpectedly. A crash log has been written to your Koala documents folder "
					"(crash.log).\n\nPlease send this file to koala.helpdesk@gmail.com");
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
	} catch (const std::exception &e) {
		writeCrashLog(std::string("Unhandled C++ exception: ") + e.what());
		showCrashDialog("Koala - Error",
						std::string("Koala crashed: ") + e.what()
							+ "\n\nA crash log has been written to your Koala documents folder (crash.log).");
	} catch (...) {
		writeCrashLog("Unknown unhandled exception");
		showCrashDialog("Koala - Error",
						"Koala crashed unexpectedly.\n\nA crash log has been written to your Koala documents "
						"folder (crash.log).");
	}
	return 0;
}

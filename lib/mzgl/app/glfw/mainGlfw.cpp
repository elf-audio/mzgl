#include "GLFWAppRunner.h"
#include <fstream>
#include <string>
#include <ctime>
#include <cstdio>

#ifdef _WIN32
#	include <windows.h>
#	include <shlobj.h>
#	include <dbghelp.h>
#	pragma comment(lib, "dbghelp.lib")

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

static std::string getVersionString() {
	std::string ver;
#ifdef KOALA_VERSION
	ver += " v" KOALA_VERSION;
#endif
#ifdef KOALA_BUILD_NUMBER
	ver += " (build " KOALA_BUILD_NUMBER ")";
#endif
#ifdef KOALA_GIT_HASH
	ver += " [" KOALA_GIT_HASH "]";
#endif
	return ver;
}

static void writeCrashLog(const std::string &msg) {
	try {
		std::ofstream f(getCrashLogPath(), std::ios::app);
		std::time_t t = std::time(nullptr);
		char timeBuf[64];
		std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
		f << "[" << timeBuf << "]" << getVersionString() << " " << msg << "\n";
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
static std::string getExceptionName(DWORD code) {
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

static void writeStackTrace(std::ofstream &f) {
	HANDLE process = GetCurrentProcess();
	SymInitialize(process, nullptr, TRUE);

	void *stack[64];
	WORD frames = CaptureStackBackTrace(2, 64, stack, nullptr);

	char symbolBuf[sizeof(SYMBOL_INFO) + 256];
	SYMBOL_INFO *symbol = reinterpret_cast<SYMBOL_INFO *>(symbolBuf);
	symbol->MaxNameLen	= 255;
	symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

	IMAGEHLP_LINE64 line;
	line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

	f << "  Stack trace (" << frames << " frames):\n";
	for (WORD i = 0; i < frames; i++) {
		DWORD64 address = reinterpret_cast<DWORD64>(stack[i]);
		char buf[512];
		if (SymFromAddr(process, address, nullptr, symbol)) {
			DWORD displacement = 0;
			if (SymGetLineFromAddr64(process, address, &displacement, &line)) {
				snprintf(buf, sizeof(buf), "    [%2d] %s (%s:%lu)", i, symbol->Name, line.FileName,
						 line.LineNumber);
			} else {
				snprintf(buf, sizeof(buf), "    [%2d] %s (0x%llX)", i, symbol->Name, address);
			}
		} else {
			snprintf(buf, sizeof(buf), "    [%2d] 0x%llX", i, address);
		}
		f << buf << "\n";
	}
	SymCleanup(process);
}

static LONG WINAPI unhandledExceptionFilter(EXCEPTION_POINTERS *info) {
	try {
		DWORD code = info->ExceptionRecord->ExceptionCode;
		std::ofstream f(getCrashLogPath(), std::ios::app);
		std::time_t t = std::time(nullptr);
		char timeBuf[64];
		std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));

		char buf[256];
		snprintf(buf, sizeof(buf), "0x%08lX (%s)", code, getExceptionName(code).c_str());
		f << "[" << timeBuf << "]" << getVersionString() << " Unhandled exception, code: " << buf << "\n";

		snprintf(buf, sizeof(buf), "  Instruction address: 0x%llX",
				 reinterpret_cast<DWORD64>(info->ExceptionRecord->ExceptionAddress));
		f << buf << "\n";

		if (code == EXCEPTION_ACCESS_VIOLATION || code == EXCEPTION_IN_PAGE_ERROR) {
			auto accessType = info->ExceptionRecord->ExceptionInformation[0];
			auto targetAddr = info->ExceptionRecord->ExceptionInformation[1];
			const char *op	= (accessType == 0) ? "reading" : (accessType == 1) ? "writing" : "executing";
			snprintf(buf, sizeof(buf), "  Fault: %s address 0x%llX", op, static_cast<DWORD64>(targetAddr));
			f << buf << "\n";
		}

		auto *ctx = info->ContextRecord;
#ifdef _WIN64
		snprintf(buf, sizeof(buf), "  Registers: RIP=0x%llX RSP=0x%llX RBP=0x%llX", ctx->Rip, ctx->Rsp,
				 ctx->Rbp);
		f << buf << "\n";
		snprintf(buf, sizeof(buf), "  RAX=0x%llX RBX=0x%llX RCX=0x%llX RDX=0x%llX", ctx->Rax, ctx->Rbx,
				 ctx->Rcx, ctx->Rdx);
		f << buf << "\n";
#else
		snprintf(buf, sizeof(buf), "  Registers: EIP=0x%08lX ESP=0x%08lX EBP=0x%08lX", ctx->Eip, ctx->Esp,
				 ctx->Ebp);
		f << buf << "\n";
#endif

		writeStackTrace(f);
		f << "\n";
	} catch (...) {
	}

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
		try {
			std::ofstream f(getCrashLogPath(), std::ios::app);
			std::time_t t = std::time(nullptr);
			char timeBuf[64];
			std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
			f << "[" << timeBuf << "]" << getVersionString() << " Unhandled C++ exception: " << e.what() << "\n";
#ifdef _WIN32
			MEMORYSTATUSEX mem;
			mem.dwLength = sizeof(mem);
			if (GlobalMemoryStatusEx(&mem)) {
				char buf[256];
				snprintf(buf, sizeof(buf),
						 "  Memory: %lu%% used, %llu MB available physical, %llu MB available virtual",
						 mem.dwMemoryLoad, mem.ullAvailPhys / (1024 * 1024),
						 mem.ullAvailVirtual / (1024 * 1024));
				f << buf << "\n";
			}
			writeStackTrace(f);
#endif
			f << "\n";
		} catch (...) {
		}
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

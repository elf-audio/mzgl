#include "GLFWAppRunner.h"
#include <fstream>
#include <string>
#include <ctime>
#include <cstdio>

#ifdef _WIN32
#	include <windows.h>
#	include <shlobj.h>
#	include <dbghelp.h>
#	include <commctrl.h>
#	include <winhttp.h>
#	pragma comment(lib, "dbghelp.lib")
#	pragma comment(lib, "comctl32.lib")
#	pragma comment(lib, "winhttp.lib")

// Crash-report upload endpoint. Token is the shared secret with receive.php — change both
// sides together if rotating. Exposed in the binary, so this only blocks casual abuse.
#	define KOALA_CRASH_HOST	 L"dev.koalasampler.com"
#	define KOALA_CRASH_PATH	 L"/win-crash/receive.php"
#	define KOALA_CRASH_TOKEN L"7168-029f-479f-b85b-fe1c"

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

#ifdef _WIN32
static std::wstring utf8ToWide(const std::string &s) {
	if (s.empty()) return {};
	int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int) s.size(), nullptr, 0);
	std::wstring w(n, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, s.data(), (int) s.size(), w.data(), n);
	return w;
}

static void revealInExplorer(const std::string &filePath) {
	std::wstring w	  = utf8ToWide(filePath);
	std::wstring args = L"/select,\"" + w + L"\"";
	ShellExecuteW(nullptr, L"open", L"explorer.exe", args.c_str(), nullptr, SW_SHOWNORMAL);
}

static void showUploadFailedDialog(const std::string &logPath, DWORD status) {
	char head[256];
	if (status == 0) {
		snprintf(head, sizeof(head),
				 "Couldn't reach the server. Please check your internet connection, or email the log "
				 "to koala.helpdesk@elf-audio.com.");
	} else {
		snprintf(head, sizeof(head),
				 "Upload failed (HTTP %lu). Sorry - please email the log to koala.helpdesk@elf-audio.com.",
				 status);
	}
	std::string content	  = std::string(head) + "\n\nLog file:\n" + logPath;
	std::wstring wContent = utf8ToWide(content);

	enum : int { BTN_OPEN = 2001 };
	const TASKDIALOG_BUTTON buttons[] = {
		{BTN_OPEN, L"Show crash log\nOpen the folder containing crash.log"},
	};

	TASKDIALOGCONFIG cfg   = {};
	cfg.cbSize			   = sizeof(cfg);
	cfg.dwFlags			   = TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION;
	cfg.pszWindowTitle	   = L"Koala";
	cfg.pszMainIcon		   = TD_WARNING_ICON;
	cfg.pszMainInstruction = L"Couldn't send crash report";
	cfg.pszContent		   = wContent.c_str();
	cfg.pButtons		   = buttons;
	cfg.cButtons		   = ARRAYSIZE(buttons);
	cfg.dwCommonButtons	   = TDCBF_CLOSE_BUTTON;

	int pressed = 0;
	if (FAILED(TaskDialogIndirect(&cfg, &pressed, nullptr, nullptr))) {
		MessageBoxA(nullptr, content.c_str(), "Koala", MB_OK | MB_ICONWARNING);
		return;
	}
	if (pressed == BTN_OPEN) {
		revealInExplorer(logPath);
	}
}

// returns HTTP status code, or 0 on transport failure
static DWORD uploadCrashReport(const std::string &logPath) {
	std::ifstream f(logPath, std::ios::binary | std::ios::ate);
	if (!f) return 0;
	auto sz = f.tellg();
	if (sz <= 0 || sz > 1024 * 1024) return 0; // mirror server cap
	f.seekg(0);
	std::string body((size_t) sz, '\0');
	f.read(body.data(), sz);
	f.close();

	DWORD status = 0;
	HINTERNET hSession = WinHttpOpen(L"Koala/1.0",
									 WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
									 WINHTTP_NO_PROXY_NAME,
									 WINHTTP_NO_PROXY_BYPASS,
									 0);
	if (!hSession) return 0;

	HINTERNET hConnect = WinHttpConnect(hSession, KOALA_CRASH_HOST, INTERNET_DEFAULT_HTTPS_PORT, 0);
	HINTERNET hRequest = nullptr;
	if (hConnect) {
		hRequest = WinHttpOpenRequest(hConnect,
									  L"POST",
									  KOALA_CRASH_PATH,
									  nullptr,
									  WINHTTP_NO_REFERER,
									  WINHTTP_DEFAULT_ACCEPT_TYPES,
									  WINHTTP_FLAG_SECURE);
	}
	if (hRequest) {
		const wchar_t *headers = L"Content-Type: text/plain; charset=utf-8\r\n"
								 L"X-Koala-Token: " KOALA_CRASH_TOKEN L"\r\n";
		if (WinHttpSendRequest(hRequest,
							   headers,
							   (DWORD) -1L,
							   (LPVOID) body.data(),
							   (DWORD) body.size(),
							   (DWORD) body.size(),
							   0)
			&& WinHttpReceiveResponse(hRequest, nullptr)) {
			DWORD len = sizeof(status);
			WinHttpQueryHeaders(hRequest,
								WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
								WINHTTP_HEADER_NAME_BY_INDEX,
								&status,
								&len,
								WINHTTP_NO_HEADER_INDEX);
		}
	}

	if (hRequest) WinHttpCloseHandle(hRequest);
	if (hConnect) WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);
	return status;
}
#endif

static void showCrashDialog(const std::string &title, const std::string &message) {
#ifdef _WIN32
	std::string logPath	  = getCrashLogPath();
	std::wstring wTitle	  = utf8ToWide(title);
	std::wstring wMessage = utf8ToWide(message);

	enum : int { BTN_SEND = 1002 };
	const TASKDIALOG_BUTTON buttons[] = {
		{BTN_SEND, L"Send crash report\nUpload the crash log so we can fix it"},
	};

	TASKDIALOGCONFIG cfg	  = {};
	cfg.cbSize				  = sizeof(cfg);
	cfg.dwFlags				  = TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION;
	cfg.pszWindowTitle		  = wTitle.c_str();
	cfg.pszMainIcon			  = TD_ERROR_ICON;
	cfg.pszMainInstruction	  = L"Koala crashed";
	cfg.pszContent			  = wMessage.c_str();
	cfg.pButtons			  = buttons;
	cfg.cButtons			  = ARRAYSIZE(buttons);
	cfg.dwCommonButtons		  = TDCBF_CLOSE_BUTTON;
	cfg.nDefaultButton		  = BTN_SEND;

	int pressed = 0;
	if (FAILED(TaskDialogIndirect(&cfg, &pressed, nullptr, nullptr))) {
		// fall back to a plain message box if TaskDialog isn't available
		MessageBoxA(nullptr, message.c_str(), title.c_str(), MB_OK | MB_ICONERROR);
		return;
	}

	if (pressed == BTN_SEND) {
		DWORD status = uploadCrashReport(logPath);
		if (status == 200) {
			MessageBoxA(nullptr, "Crash report sent. Thank you!", "Koala", MB_OK | MB_ICONINFORMATION);
		} else {
			showUploadFailedDialog(logPath, status);
		}
	}
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

		{
			HMODULE hModule = nullptr;
			GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
							   | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
							   reinterpret_cast<LPCSTR>(info->ExceptionRecord->ExceptionAddress),
							   &hModule);
			if (hModule) {
				char modName[MAX_PATH] = {};
				GetModuleFileNameA(hModule, modName, MAX_PATH);
				snprintf(buf, sizeof(buf), "  Module: %s (base: 0x%llX)",
						 modName, reinterpret_cast<DWORD64>(hModule));
			} else {
				snprintf(buf, sizeof(buf), "  Module: unknown (base: unknown)");
			}
			f << buf << "\n";
		}

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
					"Sorry - Koala crashed.");
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
							+ "\n\nA crash log has been saved.");
	} catch (...) {
		writeCrashLog("Unknown unhandled exception");
		showCrashDialog("Koala - Error",
						"Sorry - Koala crashed.");
	}
	return 0;
}

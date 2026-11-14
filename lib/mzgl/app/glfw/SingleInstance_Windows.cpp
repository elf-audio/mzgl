#include "SingleInstance_Windows.h"

#include <Windows.h>
#include <algorithm>
#include <cwctype>

namespace single_instance {

	namespace {
		// identifies our WM_COPYDATA messages ('mzSI')
		constexpr ULONG_PTR copyDataMagic = 0x6D7A5349;

		// how long a second launch waits for the first one's window to appear (it
		// can still be starting up) and for it to accept a file
		constexpr DWORD waitForWindowMs	 = 8000;
		constexpr DWORD sendTimeoutMs	 = 5000;
		constexpr DWORD windowPollStepMs = 100;

		std::function<void(const std::string &)> g_onFile;
		HWND g_mainWindow = nullptr;

		std::wstring toWide(const std::string &utf8) {
			if (utf8.empty()) return {};
			const int n = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
			std::wstring out(n > 0 ? n : 0, L'\0');
			if (n > 0) MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), out.data(), n);
			return out;
		}

		std::string toUtf8(const wchar_t *wide, size_t length) {
			if (length == 0) return {};
			const int n =
				WideCharToMultiByte(CP_UTF8, 0, wide, static_cast<int>(length), nullptr, 0, nullptr, nullptr);
			std::string out(n > 0 ? n : 0, '\0');
			if (n > 0) {
				WideCharToMultiByte(CP_UTF8, 0, wide, static_cast<int>(length), out.data(), n, nullptr, nullptr);
			}
			return out;
		}

		// The name everything is keyed on: per app (the exe's file name, so two
		// different mzgl apps don't talk to each other) and, through the Local\
		// namespace and per-session window lookup, per logged-in user session.
		std::wstring instanceName() {
			wchar_t path[MAX_PATH] = {};
			const DWORD length	   = GetModuleFileNameW(nullptr, path, MAX_PATH);
			std::wstring exe(path, length);
			const auto slash = exe.find_last_of(L"\\/");
			if (slash != std::wstring::npos) exe = exe.substr(slash + 1);
			const auto dot = exe.find_last_of(L'.');
			if (dot != std::wstring::npos) exe = exe.substr(0, dot);
			std::transform(exe.begin(), exe.end(), exe.begin(), [](wchar_t c) {
				return static_cast<wchar_t>(std::towlower(c));
			});
			return L"mzgl-single-instance-" + exe;
		}

		// the hidden window the first instance listens on - a message-only window,
		// so it can't be mistaken for (or by) any visible window, a plug-in
		// editor of the same product included
		HWND findListener() {
			return FindWindowExW(HWND_MESSAGE, nullptr, instanceName().c_str(), nullptr);
		}

		// what the user asked this launch to open: every argument that isn't a flag,
		// made absolute because the running copy has its own working directory
		std::vector<std::wstring> filesIn(const std::vector<std::string> &args) {
			std::vector<std::wstring> files;
			for (size_t i = 1; i < args.size(); i++) {
				if (args[i].empty() || args[i][0] == '-') continue;
				const auto wide = toWide(args[i]);
				wchar_t full[32768];
				const DWORD n = GetFullPathNameW(wide.c_str(), 32768, full, nullptr);
				files.push_back(n > 0 && n < 32768 ? std::wstring(full, n) : wide);
			}
			return files;
		}

		// an empty path means "nothing to open, just come to the front"
		bool sendFile(HWND listener, const std::wstring &path) {
			COPYDATASTRUCT data {};
			data.dwData = copyDataMagic;
			data.cbData = static_cast<DWORD>(path.size() * sizeof(wchar_t));
			data.lpData = const_cast<wchar_t *>(path.data());
			DWORD_PTR result = 0;
			const auto sent	 = SendMessageTimeoutW(listener,
												   WM_COPYDATA,
												   0,
												   reinterpret_cast<LPARAM>(&data),
												   SMTO_ABORTIFHUNG | SMTO_NORMAL,
												   sendTimeoutMs,
												   &result);
			return sent != 0 && result != 0;
		}

		void bringMainWindowForward() {
			if (g_mainWindow == nullptr) return;
			if (IsIconic(g_mainWindow)) ShowWindow(g_mainWindow, SW_RESTORE);
			SetForegroundWindow(g_mainWindow);
		}

		LRESULT CALLBACK listenerProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
			if (message != WM_COPYDATA) return DefWindowProcW(hwnd, message, wParam, lParam);

			const auto *data = reinterpret_cast<const COPYDATASTRUCT *>(lParam);
			if (data == nullptr || data->dwData != copyDataMagic) return FALSE;

			bringMainWindowForward();
			if (data->cbData >= sizeof(wchar_t) && data->lpData != nullptr && g_onFile) {
				// the sender is blocked in SendMessage until this returns, and its
				// buffer is only valid until then - so copy the path out, and
				// onFile is expected to queue the work rather than do it here
				const auto path =
					toUtf8(static_cast<const wchar_t *>(data->lpData), data->cbData / sizeof(wchar_t));
				g_onFile(path);
			}
			return TRUE;
		}
	} // namespace

	bool appliesTo(const std::vector<std::string> &args) {
		for (size_t i = 1; i < args.size(); i++) {
			if (!args[i].empty() && args[i][0] == '-') return false;
		}
		return true;
	}

	bool handOffToRunningInstance(const std::vector<std::string> &args) {
		if (!appliesTo(args)) return false;

		// Deliberately never released: it lives as long as the process does, and
		// Windows frees it when the process goes, however it goes.
		const auto mutexName = L"Local\\" + instanceName();
		HANDLE mutex		 = CreateMutexW(nullptr, FALSE, mutexName.c_str());
		if (mutex == nullptr || GetLastError() != ERROR_ALREADY_EXISTS) {
			return false; // we're the first (or can't tell - then just run)
		}

		// Someone else has it. Their window may not exist yet if they're still starting.
		HWND listener = nullptr;
		for (DWORD waited = 0; waited <= waitForWindowMs; waited += windowPollStepMs) {
			listener = findListener();
			if (listener != nullptr) break;
			Sleep(windowPollStepMs);
		}
		if (listener == nullptr) {
			CloseHandle(mutex);
			return false;
		}

		// only the foreground process may hand the foreground to another one
		DWORD listenerProcess = 0;
		GetWindowThreadProcessId(listener, &listenerProcess);
		if (listenerProcess != 0) AllowSetForegroundWindow(listenerProcess);

		const auto files = filesIn(args);
		bool delivered	 = true;
		if (files.empty()) {
			delivered = sendFile(listener, L"");
		} else {
			for (const auto &file: files) {
				delivered = sendFile(listener, file) && delivered;
			}
		}

		CloseHandle(mutex);
		return delivered;
	}

	void listen(void *nativeWindow, std::function<void(const std::string &)> onFile) {
		g_mainWindow = static_cast<HWND>(nativeWindow);
		g_onFile	 = std::move(onFile);

		const auto className = instanceName();
		WNDCLASSEXW windowClass {};
		windowClass.cbSize		  = sizeof(windowClass);
		windowClass.lpfnWndProc	  = listenerProc;
		windowClass.hInstance	  = GetModuleHandleW(nullptr);
		windowClass.lpszClassName = className.c_str();
		RegisterClassExW(&windowClass);

		// Owned by this thread, so its messages are dispatched by the same loop
		// that pumps the main window (glfwPollEvents).
		CreateWindowExW(
			0, className.c_str(), L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, GetModuleHandleW(nullptr), nullptr);
	}

} // namespace single_instance

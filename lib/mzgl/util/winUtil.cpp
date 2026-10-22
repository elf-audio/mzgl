#include "winUtil.h"
#include <codecvt>
#include <cstdio>
#include <locale>
#include <memory>
#include <optional>
#include <thread>
#include <mutex>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <Urlmon.h>
#include <wininet.h>
#include "filesystem.h"
#include "Rectf.h"
#include "log.h"
#include "util.h"

namespace { ///////////////////////////////////////////////////////////////////////

	// A real modal Win32 dialog (DialogBoxIndirectParam over an in-memory
	// DLGTEMPLATE), i.e. the Windows equivalent of a browser prompt()/confirm():
	// laid out in dialog units so it scales with DPI, drawn in the system
	// message font, centred over the owner, with Enter / Esc / Tab / the
	// title-bar close button handled by the dialog manager.

	struct WindowsDialogBoxResult {
		std::string text;
	};

	struct WindowsDialogBoxSetup {
		struct Button {
			std::string text;
			std::function<void(WindowsDialogBoxResult)> onClicked;
			bool isDefaultPushButton {false}; // Enter
			bool isCancel {false}; // Esc and the title-bar close button
		};
		struct TextEditor {
			std::string defaultText;
		};

		HWND parent {nullptr};
		std::string title;
		std::string text;
		std::optional<TextEditor> textEditor;
		std::vector<Button> buttons;
	};

	// Dialog units: 4 horizontal / 8 vertical units are one average character
	// cell of the dialog font, so these are the standard Windows dialog metrics.
	constexpr short DLG_MIN_WIDTH	 = 240;
	constexpr short DLG_MARGIN		 = 7;
	constexpr short BUTTON_WIDTH	 = 50;
	constexpr short BUTTON_HEIGHT	 = 14;
	constexpr short BUTTON_GAP		 = 4;
	constexpr short EDIT_HEIGHT		 = 14;
	constexpr short TEXT_LINE_HEIGHT = 8;
	constexpr short TEXT_TO_EDIT_GAP = 5;
	constexpr WORD ID_TEXT			 = 2000;
	constexpr WORD ID_EDIT			 = 2001;
	constexpr WORD ID_FIRST_BUTTON	 = 3000;

	constexpr WORD CLASS_BUTTON = 0x0080;
	constexpr WORD CLASS_EDIT	= 0x0081;
	constexpr WORD CLASS_STATIC = 0x0082;

	class DialogTemplateBuilder {
	public:
		void begin(DWORD style,
				   short cx,
				   short cy,
				   const std::wstring &title,
				   WORD fontPointSize,
				   const std::wstring &fontName) {
			write<DWORD>(style | DS_SETFONT);
			write<DWORD>(0); // extended style
			itemCountPos_ = data_.size();
			write<WORD>(0); // item count, patched in finish()
			write<short>(0);
			write<short>(0);
			write<short>(cx);
			write<short>(cy);
			write<WORD>(0); // no menu
			write<WORD>(0); // default dialog window class
			writeString(title);
			write<WORD>(fontPointSize);
			writeString(fontName);
		}

		void addItem(
			WORD classAtom, DWORD style, short x, short y, short cx, short cy, WORD id, const std::wstring &text) {
			align4();
			write<DWORD>(style | WS_CHILD | WS_VISIBLE);
			write<DWORD>(0);
			write<short>(x);
			write<short>(y);
			write<short>(cx);
			write<short>(cy);
			write<WORD>(id);
			write<WORD>(0xFFFF);
			write<WORD>(classAtom);
			writeString(text);
			write<WORD>(0); // no creation data
			itemCount_++;
		}

		const DLGTEMPLATE *finish() {
			memcpy(data_.data() + itemCountPos_, &itemCount_, sizeof(WORD));
			return reinterpret_cast<const DLGTEMPLATE *>(data_.data());
		}

	private:
		template <class T>
		void write(T v) {
			const auto p = reinterpret_cast<const uint8_t *>(&v);
			data_.insert(data_.end(), p, p + sizeof(T));
		}
		void writeString(const std::wstring &s) {
			for (wchar_t c: s)
				write<WORD>(c);
			write<WORD>(0);
		}
		void align4() {
			while (data_.size() % 4 != 0)
				data_.push_back(0);
		}

		std::vector<uint8_t> data_;
		size_t itemCountPos_ {0};
		WORD itemCount_ {0};
	};

	struct DialogFont {
		std::wstring faceName;
		WORD pointSize;
	};

	DialogFont messageFont() {
		NONCLIENTMETRICSW metrics {};
		metrics.cbSize = sizeof(metrics);
		if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0)
			&& metrics.lfMessageFont.lfHeight != 0) {
			const HDC dc  = GetDC(nullptr);
			const int dpi = GetDeviceCaps(dc, LOGPIXELSY);
			ReleaseDC(nullptr, dc);
			const int points = MulDiv(std::abs(metrics.lfMessageFont.lfHeight), 72, dpi);
			return {metrics.lfMessageFont.lfFaceName, static_cast<WORD>(std::clamp(points, 8, 14))};
		}
		return {L"Segoe UI", 9};
	}

	std::wstring getWindowText(HWND hwnd) {
		const int length = GetWindowTextLengthW(hwnd);
		if (length <= 0) return L"";
		std::vector<wchar_t> buffer(length + 1);
		GetWindowTextW(hwnd, buffer.data(), length + 1);
		return std::wstring(buffer.data());
	}

	struct DialogState {
		const WindowsDialogBoxSetup *setup;
		WindowsDialogBoxResult result;
		int chosenButton {-1};
	};

	WORD buttonId(const WindowsDialogBoxSetup::Button &button, size_t index) {
		if (button.isDefaultPushButton) return IDOK;
		if (button.isCancel) return IDCANCEL;
		return static_cast<WORD>(ID_FIRST_BUTTON + index);
	}

	int buttonIndexForCommand(const WindowsDialogBoxSetup &setup, WORD id) {
		for (size_t i = 0; i < setup.buttons.size(); i++) {
			if (buttonId(setup.buttons[i], i) == id) return static_cast<int>(i);
		}
		// Esc / close with no explicit cancel button: treat the last button as the way out
		if (id == IDCANCEL && !setup.buttons.empty()) return static_cast<int>(setup.buttons.size()) - 1;
		return -1;
	}

	// The template guesses the message height; once the real font and DPI are
	// known, measure the text and push everything below it down if needed.
	void fitMessageText(HWND dlg) {
		const HWND text = GetDlgItem(dlg, ID_TEXT);
		if (!text) return;
		RECT textRect;
		GetClientRect(text, &textRect);
		const int currentHeight = textRect.bottom - textRect.top;

		const HDC dc	   = GetDC(text);
		const HFONT font   = reinterpret_cast<HFONT>(SendMessageW(text, WM_GETFONT, 0, 0));
		const HGDIOBJ old  = SelectObject(dc, font);
		RECT measured	   = textRect;
		const auto content = getWindowText(text);
		DrawTextW(dc, content.c_str(), -1, &measured, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
		SelectObject(dc, old);
		ReleaseDC(text, dc);

		const int delta = (measured.bottom - measured.top) - currentHeight;
		if (delta <= 0) return;

		SetWindowPos(
			text, nullptr, 0, 0, textRect.right - textRect.left, currentHeight + delta, SWP_NOMOVE | SWP_NOZORDER);
		for (HWND child = GetWindow(dlg, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT)) {
			if (child == text) continue;
			RECT r;
			GetWindowRect(child, &r);
			MapWindowPoints(nullptr, dlg, reinterpret_cast<POINT *>(&r), 2);
			SetWindowPos(child, nullptr, r.left, r.top + delta, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		}
		RECT dlgRect;
		GetWindowRect(dlg, &dlgRect);
		SetWindowPos(dlg,
					 nullptr,
					 0,
					 0,
					 dlgRect.right - dlgRect.left,
					 dlgRect.bottom - dlgRect.top + delta,
					 SWP_NOMOVE | SWP_NOZORDER);
	}

	void centreOverOwner(HWND dlg) {
		const HWND owner = GetWindow(dlg, GW_OWNER);
		if (!owner) return; // DS_CENTER already put it in the middle of the screen
		RECT ownerRect, dlgRect;
		GetWindowRect(owner, &ownerRect);
		GetWindowRect(dlg, &dlgRect);
		const int w = dlgRect.right - dlgRect.left;
		const int h = dlgRect.bottom - dlgRect.top;
		int x		= ownerRect.left + ((ownerRect.right - ownerRect.left) - w) / 2;
		int y		= ownerRect.top + ((ownerRect.bottom - ownerRect.top) - h) / 2;

		// keep it on the monitor the owner is on
		MONITORINFO mi {};
		mi.cbSize = sizeof(mi);
		if (GetMonitorInfoW(MonitorFromWindow(owner, MONITOR_DEFAULTTONEAREST), &mi)) {
			x = std::clamp<int>(x, mi.rcWork.left, std::max<int>(mi.rcWork.left, mi.rcWork.right - w));
			y = std::clamp<int>(y, mi.rcWork.top, std::max<int>(mi.rcWork.top, mi.rcWork.bottom - h));
		}
		SetWindowPos(dlg, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}

	INT_PTR CALLBACK dialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		auto *state = reinterpret_cast<DialogState *>(GetWindowLongPtrW(hwnd, DWLP_USER));

		switch (msg) {
			case WM_INITDIALOG: {
				SetWindowLongPtrW(hwnd, DWLP_USER, lParam);
				fitMessageText(hwnd);
				centreOverOwner(hwnd);
				if (const HWND edit = GetDlgItem(hwnd, ID_EDIT)) {
					SendMessageW(edit, EM_SETSEL, 0, -1);
					SetFocus(edit);
					return FALSE; // we set the focus ourselves
				}
				return TRUE;
			}
			case WM_COMMAND: {
				if (!state) return FALSE;
				const int chosen = buttonIndexForCommand(*state->setup, LOWORD(wParam));
				if (chosen < 0) return FALSE;
				state->chosenButton = chosen;
				if (const HWND edit = GetDlgItem(hwnd, ID_EDIT)) {
					state->result.text = w2n(getWindowText(edit));
				}
				EndDialog(hwnd, chosen);
				return TRUE;
			}
			case WM_CLOSE: {
				// the dialog manager normally turns this into IDCANCEL; be explicit so
				// the caller always gets its callback
				SendMessageW(hwnd, WM_COMMAND, IDCANCEL, 0);
				return TRUE;
			}
		}
		return FALSE;
	}

	// Runs the dialog modally and invokes the chosen button's callback before returning.
	auto windowsDialogBox(WindowsDialogBoxSetup setup) -> void {
		// Under the app's common-controls v6 manifest this is what makes Button /
		// Edit / Static draw with visual styles instead of the Windows 95 look.
		static const bool comctl32Loaded = (LoadLibraryW(L"comctl32.dll") != nullptr);
		(void) comctl32Loaded;

		const auto font = messageFont();

		const size_t buttonCount = setup.buttons.size();
		const short buttonsWidth = static_cast<short>(buttonCount * BUTTON_WIDTH
													  + (buttonCount > 0 ? (buttonCount - 1) * BUTTON_GAP : 0));
		const short width		 = std::max<short>(DLG_MIN_WIDTH, buttonsWidth + 2 * DLG_MARGIN);
		const short contentWidth = width - 2 * DLG_MARGIN;

		// rough first guess at the message height; fitMessageText() corrects it
		short textHeight = 0;
		if (!setup.text.empty()) {
			const int charsPerLine = std::max(1, contentWidth / 5);
			int lines			   = 0;
			size_t start		   = 0;
			while (true) {
				const size_t nl	 = setup.text.find('\n', start);
				const size_t len = (nl == std::string::npos ? setup.text.size() : nl) - start;
				lines += std::max<int>(1, static_cast<int>((len + charsPerLine - 1) / charsPerLine));
				if (nl == std::string::npos) break;
				start = nl + 1;
			}
			textHeight = static_cast<short>(lines * TEXT_LINE_HEIGHT);
		}

		short y			  = DLG_MARGIN;
		const short textY = y;
		if (textHeight > 0) y += textHeight + TEXT_TO_EDIT_GAP;
		const short editY = y;
		if (setup.textEditor) y += EDIT_HEIGHT + DLG_MARGIN;
		else if (textHeight > 0) y += DLG_MARGIN - TEXT_TO_EDIT_GAP + 2;
		const short buttonsY = y;
		const short height	 = buttonsY + BUTTON_HEIGHT + DLG_MARGIN;

		DialogTemplateBuilder tpl;
		tpl.begin(WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | DS_CENTER,
				  width,
				  height,
				  n2w(setup.title),
				  font.pointSize,
				  font.faceName);
		if (textHeight > 0) {
			tpl.addItem(CLASS_STATIC,
						SS_LEFT | SS_NOPREFIX,
						DLG_MARGIN,
						textY,
						contentWidth,
						textHeight,
						ID_TEXT,
						n2w(setup.text));
		}
		if (setup.textEditor) {
			tpl.addItem(CLASS_EDIT,
						ES_LEFT | ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP,
						DLG_MARGIN,
						editY,
						contentWidth,
						EDIT_HEIGHT,
						ID_EDIT,
						n2w(setup.textEditor->defaultText));
		}
		short bx = width - DLG_MARGIN - buttonsWidth;
		for (size_t i = 0; i < buttonCount; i++) {
			const auto &button = setup.buttons[i];
			tpl.addItem(CLASS_BUTTON,
						(button.isDefaultPushButton ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON) | WS_TABSTOP,
						bx,
						buttonsY,
						BUTTON_WIDTH,
						BUTTON_HEIGHT,
						buttonId(button, i),
						n2w(button.text));
			bx += BUTTON_WIDTH + BUTTON_GAP;
		}

		DialogState state {&setup};
		const INT_PTR rc = DialogBoxIndirectParamW(
			GetModuleHandleW(nullptr), tpl.finish(), setup.parent, dialogProc, reinterpret_cast<LPARAM>(&state));
		if (rc == -1) {
			Log::e() << "DialogBoxIndirectParamW failed: " << GetLastError();
			state.chosenButton = buttonIndexForCommand(setup, IDCANCEL);
		}
		if (state.chosenButton >= 0 && state.chosenButton < static_cast<int>(buttonCount)) {
			if (const auto &cb = setup.buttons[state.chosenButton].onClicked) cb(std::move(state.result));
		}
	}

	void ensureThatFileDirectoryExists(std::wstring filePath) {
		fs::path path {filePath};
		fs::create_directories(path.parent_path());
	}

} // namespace

void windowsConfirmDialog(HWND parent,
						  std::string title,
						  std::string msg,
						  std::function<void()> okPressed,
						  std::function<void()> cancelPressed) {
	auto response = MessageBoxW(parent, n2w(msg).c_str(), n2w(title).c_str(), MB_OKCANCEL);
	if (response == IDOK) {
		okPressed();
	} else {
		cancelPressed();
	}
}

namespace {
	struct AlertThreadArgs {
		std::wstring title;
		std::wstring msg;
		HMODULE module;
	};

	DWORD WINAPI alertThreadProc(LPVOID param) {
		std::unique_ptr<AlertThreadArgs> args(static_cast<AlertThreadArgs *>(param));
		// No owner: an owner HWND would be disabled while the box is up and
		// would have to belong to this thread. SETFOREGROUND + TOPMOST so it
		// can't be lost behind the host / browser window the way an unowned
		// MB_OK was after web activation.
		MessageBoxW(nullptr, args->msg.c_str(), args->title.c_str(), MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
		HMODULE module = args->module;
		args.reset();
		if (module != nullptr) {
			// Balances the GetModuleHandleExW() in windowsAlertDialog(); the
			// DLL (a plug-in the host may unload at any time) stays mapped
			// until this thread is done with it.
			FreeLibraryAndExitThread(module, 0);
		}
		return 0;
	}
} // namespace

void windowsAlertDialog(std::string title, std::string msg) {
	auto args	 = std::make_unique<AlertThreadArgs>();
	args->title	 = n2w(title);
	args->msg	 = n2w(msg);
	args->module = nullptr;
	if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
							reinterpret_cast<LPCWSTR>(&windowsAlertDialog),
							&args->module)) {
		args->module = nullptr;
	}
	// A blocking MessageBoxW on the caller's thread runs a nested message
	// loop. From a plug-in editor's frame that freezes the UI, and if the host
	// closes the editor meanwhile the box returns into a destroyed app
	// (KoalaFX activation crash). CreateThread rather than std::thread so the
	// thread can end via FreeLibraryAndExitThread.
	HANDLE thread = CreateThread(nullptr, 0, alertThreadProc, args.get(), 0, nullptr);
	if (thread == nullptr) {
		Log::e() << "windowsAlertDialog: CreateThread failed: " << GetLastError();
		if (args->module != nullptr) {
			FreeLibrary(args->module);
		}
		return;
	}
	args.release();
	CloseHandle(thread);
}

void windowsTextboxDialog(HWND parent,
						  std::string title,
						  std::string msg,
						  std::string text,
						  std::function<void(std::string, bool)> completionCallback) {
	auto onOk	  = [completionCallback](WindowsDialogBoxResult result) { completionCallback(result.text, true); };
	auto onCancel = [completionCallback](WindowsDialogBoxResult result) {
		completionCallback(result.text, false);
	};
	WindowsDialogBoxSetup setup;
	setup.parent				  = parent;
	setup.title					  = title;
	setup.text					  = msg;
	setup.textEditor			  = WindowsDialogBoxSetup::TextEditor {};
	setup.textEditor->defaultText = text;
	setup.buttons.push_back(WindowsDialogBoxSetup::Button {"OK", onOk, true});
	setup.buttons.push_back(WindowsDialogBoxSetup::Button {"Cancel", onCancel, false, true});
	windowsDialogBox(std::move(setup));
}

void windowsTwoOptionCancelDialog(HWND parent,
								  std::string title,
								  std::string msg,
								  std::string buttonOneText,
								  std::function<void()> buttonOnePressed,
								  std::string buttonTwoText,
								  std::function<void()> buttonTwoPressed,
								  std::function<void()> cancelPressed) {
	const auto onButtonOne = [buttonOnePressed](auto &&...) { buttonOnePressed(); };
	const auto onButtonTwo = [buttonTwoPressed](auto &&...) { buttonTwoPressed(); };
	const auto onCancel	   = [cancelPressed](auto &&...) { cancelPressed(); };
	WindowsDialogBoxSetup setup;
	setup.parent = parent;
	setup.title	 = title;
	setup.text	 = msg;
	setup.buttons.push_back(WindowsDialogBoxSetup::Button {buttonOneText, onButtonOne, true});
	setup.buttons.push_back(WindowsDialogBoxSetup::Button {buttonTwoText, onButtonTwo});
	setup.buttons.push_back(WindowsDialogBoxSetup::Button {"Cancel", onCancel, false, true});
	windowsDialogBox(std::move(setup));
}

void windowsThreeOptionCancelDialog(HWND parent,
									std::string title,
									std::string msg,
									std::string buttonOneText,
									std::function<void()> buttonOnePressed,
									std::string buttonTwoText,
									std::function<void()> buttonTwoPressed,
									std::string buttonThreeText,
									std::function<void()> buttonThreePressed,
									std::function<void()> cancelPressed) {
	const auto onButtonOne	 = [buttonOnePressed](auto &&...) { buttonOnePressed(); };
	const auto onButtonTwo	 = [buttonTwoPressed](auto &&...) { buttonTwoPressed(); };
	const auto onButtonThree = [buttonThreePressed](auto &&...) { buttonThreePressed(); };
	const auto onCancel		 = [cancelPressed](auto &&...) { cancelPressed(); };
	WindowsDialogBoxSetup setup;
	setup.parent = parent;
	setup.title	 = title;
	setup.text	 = msg;
	setup.buttons.push_back(WindowsDialogBoxSetup::Button {buttonOneText, onButtonOne, true});
	setup.buttons.push_back(WindowsDialogBoxSetup::Button {buttonTwoText, onButtonTwo});
	setup.buttons.push_back(WindowsDialogBoxSetup::Button {buttonThreeText, onButtonThree});
	setup.buttons.push_back(WindowsDialogBoxSetup::Button {"Cancel", onCancel, false, true});
	windowsDialogBox(std::move(setup));
}

#include <ShObjIdl_core.h>

void windowsChooseEntryDialog(HWND parent,
							  bool isFile,
							  std::string msg,
							  std::function<void(std::string, bool)> completionCallback) {
	IFileDialog *pfd;
	bool success = false;
	WCHAR *entryName;
	if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd)))) {
		DWORD dwOptions;
		if (SUCCEEDED(pfd->GetOptions(&dwOptions))) {
			// FOS_NOCHANGEDIR: without this, the dialog moves the process CWD to
			// the selected folder, which breaks every later relative-path data load.
			pfd->SetOptions(dwOptions | FOS_NOCHANGEDIR
							| (isFile ? (FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST) : FOS_PICKFOLDERS));
		}
		if (SUCCEEDED(pfd->Show(NULL))) {
			IShellItem *psi;
			if (SUCCEEDED(pfd->GetResult(&psi))) {
				if (SUCCEEDED(psi->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &entryName))) {
					success = true;
				}
				psi->Release();
			}
		}
		pfd->Release();
	}

	std::string filename;
	if (success) {
		std::wstring ws(entryName);
		filename = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(ws);
		CoTaskMemFree(entryName);
	}
	completionCallback(success ? filename : "", success);
}

struct DownloadBSCallback : public IBindStatusCallback {
	DownloadBSCallback(std::function<void(float)> onProgress)
		: onProgress_ {onProgress} {}

	auto abort() {
		std::unique_lock lock {mutex_};
		abort_ = true;
	}

private:
	STDMETHODIMP
	OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText) override {
		if (ulProgressMax == 0) {
			return S_OK;
		}
		onProgress_(float(ulProgress) / ulProgressMax);
		std::unique_lock lock {mutex_};
		if (abort_) {
			return E_ABORT;
		}
		return S_OK;
	}

	STDMETHODIMP QueryInterface(REFIID riid, void **ppv) override { return E_NOTIMPL; }
	STDMETHODIMP_(ULONG) AddRef() override { return 1; }
	STDMETHODIMP_(ULONG) Release() override { return 1; }
	STDMETHODIMP OnStartBinding(DWORD dwReserved, IBinding *pib) override { return E_NOTIMPL; }
	STDMETHODIMP GetPriority(LONG *pnPriority) override { return E_NOTIMPL; }
	STDMETHODIMP OnLowResource(DWORD reserved) override { return E_NOTIMPL; }
	STDMETHODIMP GetBindInfo(DWORD *grfBINDF, BINDINFO *pbindinfo) { return E_NOTIMPL; }
	STDMETHODIMP OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM *pstgmed) {
		return E_NOTIMPL;
	}
	STDMETHODIMP OnStopBinding(HRESULT hresult, LPCWSTR szError) { return E_NOTIMPL; }
	STDMETHODIMP OnObjectAvailable(REFIID riid, IUnknown *punk) { return E_NOTIMPL; }

	std::function<void(float)> onProgress_;
	std::mutex mutex_;
	bool abort_ {false};
};

struct WindowsFileDownloadTask : public IWindowsFileDownloadTask {
public:
	WindowsFileDownloadTask(WindowsFileDownloadSpec spec)
		: bsCallback_ {std::move(spec.onProgress)}
		, spec_ {std::move(spec)} {
		thread_ = std::thread([url		   = spec_.url,
							   dst		   = spec_.destinationFilePath,
							   &bsCallback = bsCallback_,
							   onComplete  = spec_.onComplete,
							   onError	   = spec_.onError] {
			ensureThatFileDirectoryExists(dst);
			// urlmon fails the bind on an HTTP error status (INET_E_DOWNLOAD_FAILURE
			// and friends) but may still have written the server's error page to
			// dst, so never hand a failed transfer to the caller as the file.
			const HRESULT hr = URLDownloadToFile(NULL, url.c_str(), dst.c_str(), 0, &bsCallback);
			if (FAILED(hr)) {
				DeleteFileW(dst.c_str());
				char code[16];
				snprintf(code, sizeof(code), "0x%08lx", static_cast<unsigned long>(hr));
				Log::e() << "Download of " << w2n(url) << " failed: HRESULT " << code;
				if (onError) {
					onError(std::string {"The download failed (error "} + code + ")");
					return;
				}
			}
			onComplete();
		});
	}

	~WindowsFileDownloadTask() {
		if (thread_.joinable()) {
			bsCallback_.abort();
			thread_.join();
		}
	}

private:
	std::thread thread_;
	DownloadBSCallback bsCallback_;
	WindowsFileDownloadSpec spec_;
};

std::unique_ptr<IWindowsFileDownloadTask> windowsDownloadFile(WindowsFileDownloadSpec spec) {
	return std::make_unique<WindowsFileDownloadTask>(std::move(spec));
}

std::wstring windowsGetPathForTemporaryFile(std::wstring fileName) {
	wchar_t buffer[MAX_PATH];
	const auto len {GetTempPath(MAX_PATH, buffer)};
	assert(len != 0);
	std::wstring tempDir;
	for (DWORD i = 0; i < len; i++) {
		tempDir += buffer[i];
	}
	return tempDir + L"\\" + fileName;
}

std::string w2n(const std::wstring &w) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::string narrow = converter.to_bytes(w);
	return narrow;
}

std::wstring n2w(const std::string &n) {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring wide = converter.from_bytes(n);
	return wide;
}

fs::path getDLLPath() {
	TCHAR path[MAX_PATH];
	HMODULE hm {NULL};
	const auto flags {GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT};
	if (GetModuleHandleEx(flags, (LPTSTR) &getDLLPath, &hm) == 0) {
		return {};
	}
	if (GetModuleFileName(hm, path, sizeof(path)) == 0) {
		return {};
	}
	return fs::path {path};
}

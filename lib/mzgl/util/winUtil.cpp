#include "winUtil.h"
#include <codecvt>
#include <locale>
#include <optional>
#include <thread>
#include <mutex>
#include <vector>
#include <Urlmon.h>
#include <wininet.h>
#include "filesystem.h"
#include "Rectf.h"
#include "log.h"
#include "util.h"

namespace { ///////////////////////////////////////////////////////////////////////

	struct WindowsDialogBoxResult {
		std::string text;
	};

	struct WindowsDialogBoxSetup {
		struct Button {
			std::string text;
			std::function<void(WindowsDialogBoxResult)> onClicked;
			bool isDefaultPushButton {false};
		};
		struct TextEditor {
			std::string defaultText;
		};

		HWND parent;
		std::string title;
		std::string text;
		std::optional<TextEditor> textEditor;
		std::vector<Button> buttons;
		float x;
		float y;
	};

	struct WM {
		HWND hwnd;
		UINT msg;
		WPARAM w;
		LPARAM l;
	};

	struct DialogBoxData {
		DialogBoxData(WindowsDialogBoxSetup setup)
			: font_ {createFont()} {
			static constexpr auto BUTTON_WIDTH {120.0f};
			static constexpr auto BUTTON_HEIGHT {24.0f};
			static constexpr auto PADDING {10.0f};
			static constexpr auto TEXT_AREA_HEIGHT {40.0f};
			static constexpr auto TEXT_EDITOR_HEIGHT {24.0f};

			const auto calculateWindowHeight = [](bool hasText, bool hasTextEditor) {
				auto out {BUTTON_HEIGHT + PADDING + PADDING};
				if (hasTextEditor) {
					out += hasTextEditor + PADDING;
				}
				if (hasText) {
					out *= 2;
				}
				return out;
			};

			WORD id {100};

			const float buttonAreaWidth {(setup.buttons.size() * (BUTTON_WIDTH + PADDING)) + PADDING};

			rect_.x		 = setup.x;
			rect_.y		 = setup.y;
			rect_.width	 = buttonAreaWidth * 1.333f;
			rect_.height = calculateWindowHeight(!setup.text.empty(), setup.textEditor.has_value());

			float x {(rect_.width / 2) - (buttonAreaWidth / 2)};
			float y {rect_.height - PADDING - BUTTON_HEIGHT};

			for (const auto buttonSetup: setup.buttons) {
				Button button;
				button.id				   = id++;
				button.onClicked		   = buttonSetup.onClicked;
				button.text				   = buttonSetup.text;
				button.rect				   = {x, y, BUTTON_WIDTH, BUTTON_HEIGHT};
				button.isDefaultPushButton = buttonSetup.isDefaultPushButton;
				buttons_.push_back(std::move(button));
				x += BUTTON_WIDTH + PADDING;
			}

			y -= PADDING;

			if (setup.textEditor) {
				y -= TEXT_EDITOR_HEIGHT;
				textEditor_				 = TextEditor {};
				textEditor_->id			 = id++;
				textEditor_->defaultText = setup.textEditor->defaultText;
				textEditor_->rect.setLeftEdge(PADDING);
				textEditor_->rect.setRightEdge(rect_.width - PADDING);
				textEditor_->rect.setTopEdge(y);
				textEditor_->rect.height = TEXT_EDITOR_HEIGHT;
				y -= PADDING;
			}

			if (!setup.text.empty()) {
				text_		= Text {};
				text_->id	= id++;
				text_->text = setup.text;
				text_->rect.setLeftEdge(PADDING);
				text_->rect.setRightEdge(rect_.width - PADDING);
				text_->rect.setTopEdge(PADDING);
				text_->rect.setBottomEdge(y);
			}
		}

		~DialogBoxData() { DeleteObject(font_); }

		auto on_wm_create(WM wm) -> LRESULT {
			SetWindowLongPtr(wm.hwnd, GWLP_USERDATA, reinterpret_cast<LPARAM>(this));
			HWND textHandle {0};
			HWND textEditorHandle {0};
			std::vector<HWND> buttonHandles {};
			if (text_) {
				textHandle = createTextWindow(*text_, wm.hwnd);
			}
			if (textEditor_) {
				textEditorHandle = createTextEditorWindow(*textEditor_, wm.hwnd);
			}
			for (const auto button: buttons_) {
				buttonHandles.push_back(createButtonWindow(button, wm.hwnd));
			}
			if (textEditorHandle) {
				SetFocus(textEditorHandle);
			}
			return 0;
		}

		auto on_wm_command(WM wm) -> LRESULT {
			for (const auto button: buttons_) {
				if (LOWORD(wm.w) == button.id) {
					if (button.onClicked) {
						WindowsDialogBoxResult result;
						result.text = getEditorText();
						button.onClicked(std::move(result));
					}
					DestroyWindow(wm.hwnd);
					break;
				}
			}
			return 0;
		}

		auto getRect() const { return rect_; }

	private:
		struct Text {
			WORD id;
			std::string text;
			Rectf rect;
		};

		struct TextEditor {
			WORD id;
			std::string defaultText;
			Rectf rect;
		};

		struct Button {
			WORD id;
			std::string text;
			std::function<void(WindowsDialogBoxResult)> onClicked;
			Rectf rect;
			bool isDefaultPushButton;
		};

		static auto createFont() -> HFONT {
			NONCLIENTMETRICS metrics;
			metrics.cbSize = sizeof(NONCLIENTMETRICS);
			SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &metrics, 0);
			return CreateFontIndirect(&metrics.lfMessageFont);
		}

		auto createButtonWindow(Button button, HWND hwnd) -> HWND {
			DWORD style {WS_VISIBLE | WS_CHILD | WS_TABSTOP};
			if (button.isDefaultPushButton) {
				style |= BS_DEFPUSHBUTTON;
			}
			const auto buttonHwnd = CreateWindow(TEXT("Button"),
												 n2w(button.text).c_str(),
												 style,
												 button.rect.x,
												 button.rect.y,
												 button.rect.width,
												 button.rect.height,
												 hwnd,
												 (HMENU) (button.id),
												 NULL,
												 NULL);
			SendMessage(buttonHwnd, WM_SETFONT, (WPARAM) font_, MAKELPARAM(TRUE, 0));
			return buttonHwnd;
		}

		auto createTextEditorWindow(TextEditor textEditor, HWND hwnd) -> HWND {
			textEditorHwnd_ = CreateWindow(TEXT("Edit"),
										   n2w(textEditor.defaultText).c_str(),
										   WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP,
										   textEditor.rect.x,
										   textEditor.rect.y,
										   textEditor.rect.width,
										   textEditor.rect.height,
										   hwnd,
										   (HMENU) textEditor.id,
										   NULL,
										   NULL);
			SendMessage(textEditorHwnd_, WM_SETFONT, (WPARAM) font_, MAKELPARAM(TRUE, 0));
			return textEditorHwnd_;
		}

		auto createTextWindow(Text text, HWND hwnd) -> HWND {
			const auto textHwnd = CreateWindow(TEXT("Static"),
											   n2w(text.text).c_str(),
											   SS_CENTER | WS_CHILD | WS_VISIBLE,
											   text.rect.x,
											   text.rect.y,
											   text.rect.width,
											   text.rect.height,
											   hwnd,
											   (HMENU) (text.id),
											   NULL,
											   NULL);
			SendMessage(textHwnd, WM_SETFONT, (WPARAM) font_, MAKELPARAM(TRUE, 0));
			return textHwnd;
		}

		auto getEditorText() const -> std::string {
			if (!textEditor_) {
				return "";
			}

			int textLength_WithNUL {GetWindowTextLength(textEditorHwnd_) + 1};
			std::vector<wchar_t> text(textLength_WithNUL);
			GetWindowText(textEditorHwnd_, text.data(), textLength_WithNUL);
			char output[512];
			sprintf(output, "%ws", text.data());
			return output;
		}

		std::optional<Text> text_;
		std::optional<TextEditor> textEditor_;
		std::vector<Button> buttons_;
		Rectf rect_;
		HFONT font_;
		HWND textEditorHwnd_ {};
	};

	LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		WM wm {hwnd, msg, wParam, lParam};

		if (wm.msg == WM_CREATE) {
			const auto lpcs {reinterpret_cast<LPCREATESTRUCT>(wm.l)};
			const auto data {reinterpret_cast<DialogBoxData *>(lpcs->lpCreateParams)};
			return data->on_wm_create(wm);
		}

		const auto data {reinterpret_cast<DialogBoxData *>(GetWindowLongPtr(wm.hwnd, GWLP_USERDATA))};

		if (!data) {
			return DefWindowProc(wm.hwnd, wm.msg, wm.w, wm.l);
		}

		switch (wm.msg) {
			case WM_COMMAND: {
				return data->on_wm_command(wm);
			}
			case WM_DESTROY: {
				delete data;
				return 0;
			}
		}

		Log::d().startSavingToFile("z:/tmp/test.log", true);
		Log::d() << "here";
		Log::d().stopSavingToFile();

		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	auto windowsDialogBox(WindowsDialogBoxSetup setup) -> void {
		const auto data {new DialogBoxData(setup)};
		const auto hInstance {GetModuleHandle(NULL)};

		DWORD windowStyle {WS_CAPTION | WS_POPUPWINDOW | WS_VISIBLE};

		RECT requiredRect;
		requiredRect.left	= data->getRect().x;
		requiredRect.top	= data->getRect().y;
		requiredRect.right	= data->getRect().right();
		requiredRect.bottom = data->getRect().bottom();

		AdjustWindowRect(&requiredRect, windowStyle, false);

		WNDCLASS mainWindowClass {0};

		mainWindowClass.lpszClassName = TEXT("mzgl.winDialogBox");
		mainWindowClass.hInstance	  = hInstance;
		mainWindowClass.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
		mainWindowClass.lpfnWndProc	  = wndProc;
		mainWindowClass.hCursor		  = LoadCursor(0, IDC_ARROW);
		RegisterClass(&mainWindowClass);
		CreateWindow(mainWindowClass.lpszClassName,
					 n2w(setup.title).c_str(),
					 windowStyle,
					 requiredRect.left,
					 requiredRect.top,
					 requiredRect.right - requiredRect.left,
					 requiredRect.bottom - requiredRect.top,
					 setup.parent,
					 0,
					 hInstance,
					 data);
	}

	void ensureThatFileDirectoryExists(std::wstring filePath) {
		fs::path path {filePath};
		const auto parentPath {path.parent_path()};
		if (!fs::exists(parentPath)) {
			fs::create_directories(parentPath);
		}
	}

} // namespace

void windowsConfirmDialog(HWND parent,
						  std::string title,
						  std::string msg,
						  std::function<void()> okPressed,
						  std::function<void()> cancelPressed) {
	auto response = MessageBoxW(NULL, n2w(msg).c_str(), n2w(title).c_str(), MB_OKCANCEL);
	if (response == IDOK) {
		okPressed();
	} else {
		cancelPressed();
	}
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
	setup.x						  = 150.0f;
	setup.y						  = 150.0f;
	setup.buttons.push_back(WindowsDialogBoxSetup::Button {"OK", onOk, true});
	setup.buttons.push_back(WindowsDialogBoxSetup::Button {"Cancel", onCancel});
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
	setup.x		 = 150.0f;
	setup.y		 = 150.0f;
	setup.buttons.push_back(WindowsDialogBoxSetup::Button {buttonOneText, onButtonOne, true});
	setup.buttons.push_back(WindowsDialogBoxSetup::Button {buttonTwoText, onButtonTwo});
	setup.buttons.push_back(WindowsDialogBoxSetup::Button {"Cancel", onCancel});
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
	setup.x		 = 150.0f;
	setup.y		 = 150.0f;
	setup.buttons.push_back(WindowsDialogBoxSetup::Button {buttonOneText, onButtonOne, true});
	setup.buttons.push_back(WindowsDialogBoxSetup::Button {buttonTwoText, onButtonTwo});
	setup.buttons.push_back(WindowsDialogBoxSetup::Button {buttonThreeText, onButtonThree});
	setup.buttons.push_back(WindowsDialogBoxSetup::Button {"Cancel", onCancel});
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
			pfd->SetOptions(dwOptions | (isFile ? (FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST) : FOS_PICKFOLDERS));
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
							   onComplete  = spec_.onComplete] {
			ensureThatFileDirectoryExists(dst);
			URLDownloadToFile(NULL, url.c_str(), dst.c_str(), 0, &bsCallback);
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
	for (int i = 0; i < len; i++) {
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

auto getCurrentDllPath() -> std::filesystem::path {
	TCHAR path[MAX_PATH];
	HMODULE hm {NULL};
	const auto flags {GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT};
	if (GetModuleHandleEx(flags, (LPTSTR) &getCurrentDllPath, &hm) == 0) {
		return {};
	}
	if (GetModuleFileName(hm, path, sizeof(path)) == 0) {
		return {};
	}
	return std::filesystem::path {path};
}

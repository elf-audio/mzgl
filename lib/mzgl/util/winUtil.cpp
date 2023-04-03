#include "winUtil.h"

#define UNICODE
#define _UNICODE
#include <Windows.h>
#include <codecvt>
#include <locale>
#include <optional>
#include <vector>
#include "Rectf.h"

namespace { ///////////////////////////////////////////////////////////////////////

struct WindowsDialogBoxResult {
	std::string text;
};

struct WindowsDialogBoxSetup {
	struct Button {
		std::string text;
		std::function<void(WindowsDialogBoxResult)> onClicked;
	};
	struct TextEditor {
		std::string defaultText;
	};

	std::string title;
	std::string text;
	std::optional<TextEditor> textEditor;
	std::vector<Button> buttons;
	float x;
	float y;
};

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

		rect_.x = setup.x;
		rect_.y = setup.y;
		rect_.width = buttonAreaWidth * 1.333f;
		rect_.height = calculateWindowHeight(!setup.text.empty(), setup.textEditor.has_value());

		float x {(rect_.width / 2) - (buttonAreaWidth / 2)};
		float y {rect_.height - PADDING - BUTTON_HEIGHT};

		for (const auto buttonSetup: setup.buttons) {
			Button button;
			button.id = id++;
			button.onClicked = buttonSetup.onClicked;
			button.text = buttonSetup.text;
			button.rect = {x, y, BUTTON_WIDTH, BUTTON_HEIGHT};
			buttons_.push_back(std::move(button));
			x += BUTTON_WIDTH + PADDING;
		}

		y -= PADDING;

		if (setup.textEditor) {
			y -= TEXT_EDITOR_HEIGHT;
			textEditor_ = TextEditor {};
			textEditor_->id = id++;
			textEditor_->defaultText = setup.textEditor->defaultText;
			textEditor_->rect.setLeftEdge(PADDING);
			textEditor_->rect.setRightEdge(rect_.width - PADDING);
			textEditor_->rect.setTopEdge(y);
			textEditor_->rect.height = TEXT_EDITOR_HEIGHT;
			y -= PADDING;
		}

		if (!setup.text.empty()) {
			text_ = Text {};
			text_->id = id++;
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
		if (text_) {
			createTextWindow(*text_, wm.hwnd);
		}
		if (textEditor_) {
			createTextEditorWindow(*textEditor_, wm.hwnd);
		}
		for (const auto button: buttons_) {
			createButtonWindow(button, wm.hwnd);
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
	};

	static auto createFont() -> HFONT {
		NONCLIENTMETRICS metrics;
		metrics.cbSize = sizeof(NONCLIENTMETRICS);
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &metrics, 0);
		return CreateFontIndirect(&metrics.lfMessageFont);
	}

	auto createButtonWindow(Button button, HWND hwnd) -> void {
		const auto buttonHwnd = CreateWindow(TEXT("Button"),
											 n2w(button.text).c_str(),
											 WS_VISIBLE | WS_CHILD,
											 button.rect.x,
											 button.rect.y,
											 button.rect.width,
											 button.rect.height,
											 hwnd,
											 (HMENU) (button.id),
											 NULL,
											 NULL);
		SendMessage(buttonHwnd, WM_SETFONT, (WPARAM) font_, MAKELPARAM(TRUE, 0));
	}

	auto createTextEditorWindow(TextEditor textEditor, HWND hwnd) -> void {
		textEditorHwnd_ = CreateWindow(TEXT("Edit"),
												 n2w(textEditor.defaultText).c_str(),
												 WS_CHILD | WS_VISIBLE | WS_BORDER,
												 textEditor.rect.x,
												 textEditor.rect.y,
												 textEditor.rect.width,
												 textEditor.rect.height,
												 hwnd,
												 (HMENU) textEditor.id,
												 NULL,
												 NULL);
		SendMessage(textEditorHwnd_, WM_SETFONT, (WPARAM) font_, MAKELPARAM(TRUE, 0));
	}

	auto createTextWindow(Text text, HWND hwnd) -> void {
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
	}

	auto getEditorText() const -> std::string {
		if (!textEditor_) {
			return "";
		}

		int textLength_WithNUL {GetWindowTextLength(textEditorHwnd_) + 1};
		std::vector<TCHAR> text(textLength_WithNUL);
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

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

auto windowsDialogBox(WindowsDialogBoxSetup setup) -> void {
	const auto data {new DialogBoxData(setup)};
	const auto hInstance {GetModuleHandle(NULL)};

	DWORD windowStyle {WS_CAPTION | WS_POPUPWINDOW | WS_VISIBLE};

	RECT requiredRect;
	requiredRect.left = data->getRect().x;
	requiredRect.top = data->getRect().y;
	requiredRect.right = data->getRect().right();
	requiredRect.bottom = data->getRect().bottom();

	AdjustWindowRect(&requiredRect, windowStyle, false);

	WNDCLASS mainWindowClass {0};

	mainWindowClass.lpszClassName = TEXT("mzgl.winDialogBox");
	mainWindowClass.hInstance = hInstance;
	mainWindowClass.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
	mainWindowClass.lpfnWndProc = wndProc;
	mainWindowClass.hCursor = LoadCursor(0, IDC_ARROW);
	RegisterClass(&mainWindowClass);
	CreateWindow(mainWindowClass.lpszClassName,
				 n2w(setup.title).c_str(),
				 windowStyle,
				 requiredRect.left,
				 requiredRect.top,
				 requiredRect.right - requiredRect.left,
				 requiredRect.bottom - requiredRect.top,
				 NULL,
				 0,
				 hInstance,
				 data);
}

} /////////////////////////////////////////////////////////////////////////////////

void windowsConfirmDialog(std::string title, std::string msg, std::function<void()> okPressed, std::function<void()> cancelPressed) {
	auto response = MessageBoxW(NULL, n2w(msg).c_str(), n2w(title).c_str(), MB_OKCANCEL);
	if (response == IDOK) {
		okPressed();
	} else {
		cancelPressed();
	}
}

void windowsTextboxDialog(std::string title, std::string msg, std::string text, std::function<void(std::string, bool)> completionCallback) {
	auto onOk = [completionCallback](WindowsDialogBoxResult result) {
		completionCallback(result.text, true);
	};
	auto onCancel = [completionCallback](WindowsDialogBoxResult result) {
		completionCallback(result.text, false);
	};
	WindowsDialogBoxSetup setup;
	setup.title = title;
	setup.text = msg;
	setup.textEditor = WindowsDialogBoxSetup::TextEditor{};
	setup.textEditor->defaultText = text;
	setup.x = 150.0f;
	setup.y = 150.0f;
	setup.buttons.push_back(WindowsDialogBoxSetup::Button {"OK", onOk});
	setup.buttons.push_back(WindowsDialogBoxSetup::Button {"Cancel", onCancel});
	windowsDialogBox(std::move(setup));
}

void windowsTwoOptionCancelDialog(std::string title,
								  std::string msg,
								  std::string buttonOneText,
								  std::function<void()> buttonOnePressed,
								  std::string buttonTwoText,
								  std::function<void()> buttonTwoPressed,
								  std::function<void()> cancelPressed) {
	const auto onButtonOne = [buttonOnePressed](auto&&...) { buttonOnePressed(); };
	const auto onButtonTwo = [buttonTwoPressed](auto&&...) { buttonTwoPressed(); };
	const auto onCancel = [cancelPressed](auto&&...) { cancelPressed(); };
	WindowsDialogBoxSetup setup;
	setup.title = title;
	setup.text = msg;
	setup.x = 150.0f;
	setup.y = 150.0f;
	setup.buttons.push_back(WindowsDialogBoxSetup::Button {buttonOneText, onButtonOne});
	setup.buttons.push_back(WindowsDialogBoxSetup::Button {buttonTwoText, onButtonTwo});
	setup.buttons.push_back(WindowsDialogBoxSetup::Button {"Cancel", onCancel});
	windowsDialogBox(std::move(setup));
}

void windowsThreeOptionCancelDialog(std::string title,
									std::string msg,
									std::string buttonOneText,
									std::function<void()> buttonOnePressed,
									std::string buttonTwoText,
									std::function<void()> buttonTwoPressed,
									std::string buttonThreeText,
									std::function<void()> buttonThreePressed,
									std::function<void()> cancelPressed)
{
	const auto onButtonOne = [buttonOnePressed](auto&&...) { buttonOnePressed(); };
	const auto onButtonTwo = [buttonTwoPressed](auto&&...) { buttonTwoPressed(); };
	const auto onButtonThree = [buttonThreePressed](auto&&...) { buttonThreePressed(); };
	const auto onCancel = [cancelPressed](auto&&...) { cancelPressed(); };
	WindowsDialogBoxSetup setup;
	setup.title = title;
	setup.text = msg;
	setup.x = 150.0f;
	setup.y = 150.0f;
	setup.buttons.push_back(WindowsDialogBoxSetup::Button {buttonOneText, onButtonOne});
	setup.buttons.push_back(WindowsDialogBoxSetup::Button {buttonTwoText, onButtonTwo});
	setup.buttons.push_back(WindowsDialogBoxSetup::Button {buttonThreeText, onButtonThree});
	setup.buttons.push_back(WindowsDialogBoxSetup::Button {"Cancel", onCancel});
	windowsDialogBox(std::move(setup));
}

#include <ShObjIdl_core.h>

void windowsChooseEntryDialog(bool isFile, std::string msg, std::function<void(std::string, bool)> completionCallback) {
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
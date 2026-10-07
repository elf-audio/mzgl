//
//  WindowsWebView.cpp
//  mzgl
//
//  WebView2 overlay - see WindowsWebView.h. Only compiled when the WebView2 SDK
//  was found (MZGL_HAS_WEBVIEW2); otherwise the Dialogs fall back to the system
//  browser.
//

#include "WindowsWebView.h"
#include "App.h"
#include "ScopedUrl.h"
#include "log.h"
#include "util.h"
#include "winUtil.h"

#include <Windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <wrl.h>
#include <WebView2.h>

#include <filesystem>
#include <vector>

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

namespace {
	constexpr const wchar_t *overlayClassName = L"MZGLWebViewOverlay";
	constexpr UINT WM_WEBVIEW_CLOSE			  = WM_APP + 0x57; // posted to the overlay to tear itself down
	constexpr UINT_PTR parentSubclassId		  = 0x57455642; // 'WEVB'
	constexpr int closeButtonId				  = 1;

	// Lets pages written against WKWebView's message handlers (see sendMessage()
	// in www/js/utils.js) work unchanged: both handlers forward to WebView2's
	// postMessage, and the app treats "close" as a request to dismiss the view.
	constexpr const wchar_t *webkitShimJs =
		L"window.webkit = { messageHandlers: {"
		L"  messages:    { postMessage: m => window.chrome.webview.postMessage(m) },"
		L"  closeWindow: { postMessage: m => window.chrome.webview.postMessage('close') }"
		L"} };";

	std::wstring toLower(std::wstring s) {
		for (auto &c: s) {
			c = static_cast<wchar_t>(towlower(c));
		}
		return s;
	}

	bool startsWith(const std::wstring &s, const std::wstring &prefix) {
		return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
	}

	// Scheme of a uri, lowercase, without the colon - "" if there isn't one.
	std::wstring schemeOf(const std::wstring &uri) {
		auto colon = uri.find(L':');
		if (colon == std::wstring::npos) return L"";
		return toLower(uri.substr(0, colon));
	}

	// WebView2 needs a writable profile dir - next to the exe (its default) isn't
	// when installed under Program Files, so use %LOCALAPPDATA%\<exe name>\WebView2.
	std::wstring userDataFolder() {
		std::wstring base;
		PWSTR localAppData = nullptr;
		if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &localAppData))) {
			base = localAppData;
			CoTaskMemFree(localAppData);
		} else {
			base = std::filesystem::temp_directory_path().wstring();
		}
		wchar_t exePath[MAX_PATH] = {};
		GetModuleFileNameW(nullptr, exePath, MAX_PATH);
		auto exeName = std::filesystem::path(exePath).stem().wstring();
		if (exeName.empty()) exeName = L"mzgl";
		return base + L"\\" + exeName + L"\\WebView2";
	}

	std::string hex(HRESULT hr) {
		char buf[16];
		snprintf(buf, sizeof(buf), "0x%08lx", static_cast<unsigned long>(hr));
		return buf;
	}

	UINT dpiForWindow(HWND hwnd) {
		// GetDpiForWindow needs a Win10 SDK target; we build for 0x0601 so look it up.
		using GetDpiForWindowFn = UINT(WINAPI *)(HWND);
		static auto fn			= reinterpret_cast<GetDpiForWindowFn>(
			 GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));
		if (fn != nullptr) return fn(hwnd);
		HDC dc	 = GetDC(hwnd);
		UINT dpi = static_cast<UINT>(GetDeviceCaps(dc, LOGPIXELSX));
		ReleaseDC(hwnd, dc);
		return dpi == 0 ? 96 : dpi;
	}
} // namespace

class WindowsWebViewImpl : public std::enable_shared_from_this<WindowsWebViewImpl> {
public:
	WindowsWebViewImpl(App &app, WindowsWebView::Options options)
		: app(app)
		, options(std::move(options)) {}

	~WindowsWebViewImpl() { close(false); }

	// Creates the overlay window and kicks off WebView2 creation. Returns false
	// if the overlay couldn't be made at all (no parent window).
	bool open() {
		parent = static_cast<HWND>(app.nativeWindowHandle);
		if (parent == nullptr) {
			Log::e() << "WindowsWebView: no native window handle";
			return false;
		}
		registerClass();

		RECT client {};
		GetClientRect(parent, &client);
		overlay = CreateWindowExW(0,
								  overlayClassName,
								  L"",
								  WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
								  0,
								  0,
								  client.right - client.left,
								  client.bottom - client.top,
								  parent,
								  nullptr,
								  GetModuleHandleW(nullptr),
								  nullptr);
		if (overlay == nullptr) {
			Log::e() << "WindowsWebView: CreateWindowEx failed: " << GetLastError();
			return false;
		}
		SetWindowLongPtrW(overlay, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
		SetWindowSubclass(parent, parentSubclassProc, parentSubclassId, reinterpret_cast<DWORD_PTR>(this));

		if (options.showCloseBar) createCloseButton();
		layout();
		SetWindowPos(overlay, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

		startWebView2();
		return true;
	}

	void callJs(const std::string &js) {
		if (webview == nullptr) {
			// not ready yet - replay once the page has been created
			pendingJs.push_back(n2w(js));
			return;
		}
		webview->ExecuteScript(n2w(js).c_str(), nullptr);
	}

	// Safe to call from inside WebView2 event handlers - the actual teardown
	// happens on the next message pump.
	void requestClose() {
		if (closed || overlay == nullptr) return;
		PostMessageW(overlay, WM_WEBVIEW_CLOSE, 0, 0);
	}

	void close(bool notify) {
		if (closed) return;
		closed = true;
		// keep ourselves alive while onClosed may drop the owning shared_ptr
		auto self = weak_from_this().lock();

		if (webview != nullptr) {
			webview->remove_NavigationStarting(navigationToken);
			webview->remove_NewWindowRequested(newWindowToken);
			webview->remove_WebMessageReceived(messageToken);
			webview->remove_NavigationCompleted(navCompletedToken);
		}
		if (controller != nullptr) {
			controller->Close();
		}
		webview		= nullptr;
		controller	= nullptr;
		environment = nullptr;

		if (parent != nullptr) {
			RemoveWindowSubclass(parent, parentSubclassProc, parentSubclassId);
		}
		if (overlay != nullptr) {
			SetWindowLongPtrW(overlay, GWLP_USERDATA, 0);
			DestroyWindow(overlay);
			overlay		= nullptr;
			closeButton = nullptr;
		}
		if (font != nullptr) {
			DeleteObject(font);
			font = nullptr;
		}
		if (parent != nullptr) {
			SetFocus(parent);
		}
		if (notify && options.onClosed) {
			options.onClosed();
		}
	}

private:
	App &app;
	WindowsWebView::Options options;

	HWND parent		 = nullptr;
	HWND overlay	 = nullptr;
	HWND closeButton = nullptr;
	HFONT font		 = nullptr;
	bool closed		 = false;

	ComPtr<ICoreWebView2Environment> environment;
	ComPtr<ICoreWebView2Controller> controller;
	ComPtr<ICoreWebView2> webview;
	EventRegistrationToken navigationToken {};
	EventRegistrationToken newWindowToken {};
	EventRegistrationToken messageToken {};
	EventRegistrationToken navCompletedToken {};
	std::vector<std::wstring> pendingJs;

	static void registerClass() {
		static bool registered = false;
		if (registered) return;
		registered = true;
		WNDCLASSW wc {};
		wc.lpfnWndProc	 = overlayProc;
		wc.hInstance	 = GetModuleHandleW(nullptr);
		wc.hCursor		 = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
		wc.lpszClassName = overlayClassName;
		RegisterClassW(&wc);
	}

	int barHeight() const {
		return options.showCloseBar ? MulDiv(40, static_cast<int>(dpiForWindow(parent)), 96) : 0;
	}
	int padding() const { return MulDiv(6, static_cast<int>(dpiForWindow(parent)), 96); }

	void createCloseButton() {
		closeButton = CreateWindowExW(0,
									  L"BUTTON",
									  L"\u2715",
									  WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_FLAT,
									  0,
									  0,
									  0,
									  0,
									  overlay,
									  reinterpret_cast<HMENU>(static_cast<INT_PTR>(closeButtonId)),
									  GetModuleHandleW(nullptr),
									  nullptr);
		font		= CreateFontW(-MulDiv(14, static_cast<int>(dpiForWindow(parent)), 72),
							  0,
							  0,
							  0,
							  FW_NORMAL,
							  FALSE,
							  FALSE,
							  FALSE,
							  DEFAULT_CHARSET,
							  OUT_DEFAULT_PRECIS,
							  CLIP_DEFAULT_PRECIS,
							  CLEARTYPE_QUALITY,
							  DEFAULT_PITCH,
							  L"Segoe UI Symbol");
		SendMessageW(closeButton, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
	}

	// Overlay fills the parent's client area; close button top-right in the bar,
	// web view below the bar.
	void layout() {
		if (overlay == nullptr) return;
		RECT client {};
		GetClientRect(parent, &client);
		const int w = client.right - client.left;
		const int h = client.bottom - client.top;
		MoveWindow(overlay, 0, 0, w, h, TRUE);

		const int bar	= barHeight();
		const int pad	= padding();
		const int bSize = bar - pad * 2;
		if (closeButton != nullptr) {
			MoveWindow(closeButton, w - bSize - pad, pad, bSize, bSize, TRUE);
		}
		if (controller != nullptr) {
			RECT bounds {0, bar, w, h};
			controller->put_Bounds(bounds);
		}
	}

	void startWebView2() {
		// GLFW already initialised COM on this thread; this is a no-op/S_FALSE then.
		CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

		std::weak_ptr<WindowsWebViewImpl> weak = weak_from_this();
		HRESULT hr							   = CreateCoreWebView2EnvironmentWithOptions(
			nullptr,
			userDataFolder().c_str(),
			nullptr,
			Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
				[weak](HRESULT result, ICoreWebView2Environment *env) -> HRESULT {
					auto self = weak.lock();
					if (self == nullptr || self->closed) return S_OK;
					if (FAILED(result) || env == nullptr) {
						Log::e() << "WindowsWebView: environment creation failed: " << hex(result);
						self->fallbackToBrowser();
						return S_OK;
					}
					self->environment = env;
					env->CreateCoreWebView2Controller(
						self->overlay,
						Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
							[weak](HRESULT result, ICoreWebView2Controller *ctrl) -> HRESULT {
								auto self = weak.lock();
								if (self == nullptr || self->closed) return S_OK;
								if (FAILED(result) || ctrl == nullptr) {
									Log::e() << "WindowsWebView: controller creation failed: " << hex(result);
									self->fallbackToBrowser();
									return S_OK;
								}
								self->onControllerCreated(ctrl);
								return S_OK;
							})
							.Get());
					return S_OK;
				})
				.Get());
		if (FAILED(hr)) {
			// Typically the WebView2 runtime isn't installed.
			Log::e() << "WindowsWebView: CreateCoreWebView2EnvironmentWithOptions failed: " << hex(hr);
			fallbackToBrowser();
		}
	}

	void onControllerCreated(ICoreWebView2Controller *ctrl) {
		controller = ctrl;
		controller->get_CoreWebView2(&webview);
		if (webview == nullptr) {
			fallbackToBrowser();
			return;
		}

		ComPtr<ICoreWebView2Settings> settings;
		if (SUCCEEDED(webview->get_Settings(&settings)) && settings != nullptr) {
			settings->put_IsStatusBarEnabled(FALSE);
#ifndef DEBUG
			settings->put_AreDevToolsEnabled(FALSE);
#endif
			if (!options.userAgentSuffix.empty()) {
				ComPtr<ICoreWebView2Settings2> settings2;
				if (SUCCEEDED(settings.As(&settings2)) && settings2 != nullptr) {
					LPWSTR ua = nullptr;
					if (SUCCEEDED(settings2->get_UserAgent(&ua)) && ua != nullptr) {
						std::wstring newUa = std::wstring(ua) + L" " + n2w(options.userAgentSuffix);
						CoTaskMemFree(ua);
						settings2->put_UserAgent(newUa.c_str());
					}
				}
			}
		}

		std::weak_ptr<WindowsWebViewImpl> weak = weak_from_this();

		webview->AddScriptToExecuteOnDocumentCreated(webkitShimJs, nullptr);

		webview->add_NavigationStarting(
			Callback<ICoreWebView2NavigationStartingEventHandler>(
				[weak](ICoreWebView2 *, ICoreWebView2NavigationStartingEventArgs *args) -> HRESULT {
					if (auto self = weak.lock()) self->onNavigationStarting(args);
					return S_OK;
				})
				.Get(),
			&navigationToken);

		// target=_blank etc - there's no second window to open in, so hand it to
		// the system browser.
		webview->add_NewWindowRequested(
			Callback<ICoreWebView2NewWindowRequestedEventHandler>(
				[](ICoreWebView2 *, ICoreWebView2NewWindowRequestedEventArgs *args) -> HRESULT {
					LPWSTR uri = nullptr;
					if (SUCCEEDED(args->get_Uri(&uri)) && uri != nullptr) {
						launchUrl(w2n(uri));
						CoTaskMemFree(uri);
					}
					args->put_Handled(TRUE);
					return S_OK;
				})
				.Get(),
			&newWindowToken);

		webview->add_NavigationCompleted(
			Callback<ICoreWebView2NavigationCompletedEventHandler>(
				[](ICoreWebView2 *, ICoreWebView2NavigationCompletedEventArgs *args) -> HRESULT {
					BOOL ok							  = FALSE;
					COREWEBVIEW2_WEB_ERROR_STATUS err = COREWEBVIEW2_WEB_ERROR_STATUS_UNKNOWN;
					args->get_IsSuccess(&ok);
					args->get_WebErrorStatus(&err);
					if (!ok) {
						Log::e() << "WindowsWebView: navigation failed, error status " << static_cast<int>(err);
					}
					return S_OK;
				})
				.Get(),
			&navCompletedToken);

		webview->add_WebMessageReceived(
			Callback<ICoreWebView2WebMessageReceivedEventHandler>(
				[weak](ICoreWebView2 *, ICoreWebView2WebMessageReceivedEventArgs *args) -> HRESULT {
					if (auto self = weak.lock()) self->onWebMessage(args);
					return S_OK;
				})
				.Get(),
			&messageToken);

		layout();
		controller->put_IsVisible(TRUE);
		Log::d() << "WindowsWebView: ready, loading " << (options.html.empty() ? options.url : "inline html");

		if (!options.html.empty()) {
			webview->NavigateToString(n2w(options.html).c_str());
		} else {
			webview->Navigate(navigableUri(options.url).c_str());
		}
		controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);

		for (const auto &js: pendingJs) {
			webview->ExecuteScript(js.c_str(), nullptr);
		}
		pendingJs.clear();
	}

	// http(s)/other uris pass through; a bare local path becomes a file:/// uri.
	static std::wstring navigableUri(const std::string &url) {
		std::wstring w = n2w(url);
		if (w.find(L"://") != std::wstring::npos) return w;
		for (auto &c: w) {
			if (c == L'\\') c = L'/';
		}
		if (!startsWith(w, L"/")) w = L"/" + w;
		return L"file://" + w;
	}

	void onNavigationStarting(ICoreWebView2NavigationStartingEventArgs *args) {
		LPWSTR rawUri = nullptr;
		if (FAILED(args->get_Uri(&rawUri)) || rawUri == nullptr) return;
		std::wstring uri(rawUri);
		CoTaskMemFree(rawUri);
		const auto scheme = schemeOf(uri);

		if (!options.appUrlScheme.empty() && scheme == toLower(n2w(options.appUrlScheme))) {
			args->put_Cancel(TRUE);
			std::string u = w2n(uri);
			// "Open in Koala" on a koalaverse sample: close the view, then let the
			// app import it. Other app-scheme actions keep the view open.
			if (u.find(options.appUrlScheme + "://koalaverse/sample") == 0) {
				requestClose();
			}
			app.openUrl(ScopedUrl::create(u));
			return;
		}

		if (options.openLinksInBrowser) {
			// NavigateToString shows up here as a data: uri (WKWebView says about:blank)
			const bool isInternal = scheme == L"about" || scheme == L"data" || scheme == L"file"
									|| uri.find(L"://localhost") != std::wstring::npos;
			if (!isInternal) {
				args->put_Cancel(TRUE);
				launchUrl(w2n(uri));
			}
		}
	}

	void onWebMessage(ICoreWebView2WebMessageReceivedEventArgs *args) {
		LPWSTR raw = nullptr;
		std::string msg;
		if (SUCCEEDED(args->TryGetWebMessageAsString(&raw)) && raw != nullptr) {
			msg = w2n(raw);
		} else if (SUCCEEDED(args->get_WebMessageAsJson(&raw)) && raw != nullptr) {
			msg = w2n(raw);
		}
		if (raw != nullptr) CoTaskMemFree(raw);

		if (msg == "close") {
			requestClose();
			return;
		}
		if (options.jsCallback) options.jsCallback(msg);
	}

	// WebView2 couldn't be created (runtime missing or broken) - do what the
	// browser-only fallback does: open the content in the default browser.
	void fallbackToBrowser() {
		Log::e() << "WindowsWebView: falling back to the system browser";
		if (!options.html.empty()) {
			auto path = w2n((std::filesystem::temp_directory_path() / L"index.html").wstring());
			if (writeStringToFile(path, options.html)) launchUrl(path);
		} else if (!options.url.empty()) {
			launchUrl(options.url);
		}
		requestClose();
	}

	static LRESULT CALLBACK overlayProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		auto *self = reinterpret_cast<WindowsWebViewImpl *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
		switch (msg) {
			case WM_COMMAND:
				if (self != nullptr && LOWORD(wParam) == closeButtonId) {
					self->requestClose();
					return 0;
				}
				break;
			case WM_WEBVIEW_CLOSE:
				// close() may destroy self (via onClosed) - don't touch it after.
				if (self != nullptr) self->close(true);
				return 0;
			default: break;
		}
		return DefWindowProcW(hwnd, msg, wParam, lParam);
	}

	// Hooked onto the app window so the overlay tracks its size.
	static LRESULT CALLBACK
		parentSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR refData) {
		if (msg == WM_SIZE) {
			auto *self = reinterpret_cast<WindowsWebViewImpl *>(refData);
			if (self != nullptr && !self->closed) self->layout();
		}
		return DefSubclassProc(hwnd, msg, wParam, lParam);
	}
};

WindowsWebView::WindowsWebView(App &app, Options options)
	: impl(std::make_shared<WindowsWebViewImpl>(app, std::move(options))) {
	impl->open();
}

WindowsWebView::~WindowsWebView() {
	impl->close(false);
}

void WindowsWebView::callJs(const std::string &js) {
	impl->callJs(js);
}

void WindowsWebView::close() {
	impl->close(true);
}

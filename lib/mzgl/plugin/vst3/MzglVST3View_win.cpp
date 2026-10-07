// Windows IPlugView: a child HWND of the host's window, rendered with
// sokol/D3D11 off a WM_TIMER. The macOS counterpart is MzglVST3View.mm.
#ifdef _WIN32

#include "MzglVST3View.h"

#include "Plugin.h"
#include "PluginEditor.h"
#include "Graphics.h"
#include "EventDispatcher.h"
#include "util.h"
#include "winUtil.h"
#include "filesystem.h"
#include "log.h"

#include <windows.h>
#include <windowsx.h>

#ifndef MZGL_SOKOL
#	error "The Windows VST3 editor view renders with sokol/D3D11 - build with MZGL_GRAPHICS_BACKEND=Sokol"
#endif
#include "D3D11Context.h"
#include "sokol_gfx.h"
#include "SokolSetup.h"

#include <cstring>

using namespace Steinberg;

// The DLL's own HINSTANCE (not the host's) for the window class.
extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace mzglvst {

namespace {
	constexpr wchar_t kWindowClassName[] = L"MzglVST3View";
	constexpr UINT_PTR kFrameTimerId	 = 1;
	constexpr UINT kFrameIntervalMs		 = 16;

	HINSTANCE dllInstance() {
		return reinterpret_cast<HINSTANCE>(&__ImageBase);
	}

	// sokol has exactly one sg_setup() per DLL, bound to one D3D11 device. The
	// first editor opened creates the device; later editors (another instance
	// of the plugin in the same host, or the editor reopened) share it and only
	// get their own swap chain. Neither the device nor sokol is ever torn down:
	// mzgl caches sokol resources in statics that would dangle if it were.
	struct SharedGpu {
		ID3D11Device *device = nullptr;
	};
	SharedGpu &sharedGpu() {
		static SharedGpu gpu;
		return gpu;
	}

	// Tell mzgl where data/ is inside our bundle. libmzgl was built without
	// MZGL_PLUGIN_VST, so its baked-in dataPath() resolves relative to the host
	// exe. The bundle layout is <Name>.vst3/Contents/x86_64-win/<Name>.vst3 with
	// data in <Name>.vst3/Contents/Resources/data.
	void overrideDataPathToBundleResources() {
		const fs::path dll = getDLLPath();
		if (dll.empty()) return;
		const fs::path data = dll.parent_path().parent_path() / "Resources" / "data";
		if (!fs::is_directory(data)) {
			Log::e() << "MzglVST3View: no data folder at " << data.string();
		}
		setDataPath(data.string());
	}

	// Physical px per logical px for the monitor the window is on. Resolved at
	// runtime: GetDpiForWindow is Win10 1607+ and mzgl targets Win7 headers.
	float dpiScaleForWindow(HWND hwnd) {
		using GetDpiForWindowFn = UINT(WINAPI *)(HWND);
		static const auto fn	= [] {
			   const HMODULE user32 = GetModuleHandleW(L"user32.dll");
			   return user32 ? reinterpret_cast<GetDpiForWindowFn>(GetProcAddress(user32, "GetDpiForWindow"))
							 : nullptr;
		}();
		if (fn == nullptr) return 1.f;
		const UINT dpi = fn(hwnd);
		return dpi > 0 ? dpi / 96.f : 1.f;
	}

	// Same mapping as GLFWAppRunner's convertGlfwKeyToMzgl: letters/digits are
	// their (uppercase) ASCII in both VK codes and GLFW keys.
	int vkToMzglKey(WPARAM vk) {
		switch (vk) {
			case VK_LEFT: return MZ_KEY_LEFT;
			case VK_RIGHT: return MZ_KEY_RIGHT;
			case VK_DOWN: return MZ_KEY_DOWN;
			case VK_UP: return MZ_KEY_UP;
			case VK_BACK:
			case VK_DELETE: return MZ_KEY_DELETE;
			case VK_TAB: return MZ_KEY_TAB;
			case VK_ESCAPE: return MZ_KEY_ESCAPE;
			case VK_RETURN: return MZ_KEY_RETURN;
			case VK_SHIFT:
			case VK_LSHIFT:
			case VK_RSHIFT: return MZ_KEY_SHIFT;
			case VK_CONTROL:
			case VK_LCONTROL:
			case VK_RCONTROL: return MZ_KEY_CTRL;
			case VK_MENU:
			case VK_LMENU:
			case VK_RMENU: return MZ_KEY_ALT;
			case VK_LWIN:
			case VK_RWIN: return MZ_KEY_CMD;
			case VK_PRIOR: return MZ_KEY_INCREMENT;
			case VK_NEXT: return MZ_KEY_DECREMENT;
			default: return static_cast<int>(vk);
		}
	}

	std::string utf16ToUtf8(const std::wstring &w) {
		if (w.empty()) return {};
		const int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int) w.size(), nullptr, 0, nullptr, nullptr);
		std::string s(n, '\0');
		WideCharToMultiByte(CP_UTF8, 0, w.data(), (int) w.size(), s.data(), n, nullptr, nullptr);
		return s;
	}

	sg_pass makePass(const D3D11Context &d3d11, int width, int height) {
		sg_pass pass					  = {};
		pass.action.colors[0].load_action = SG_LOADACTION_CLEAR;
		pass.action.colors[0].clear_value = {0.f, 0.f, 0.f, 1.f};
		pass.swapchain.width			  = width;
		pass.swapchain.height			  = height;
		pass.swapchain.sample_count		  = d3d11.getSampleCount();
		pass.swapchain.color_format		  = SG_PIXELFORMAT_BGRA8;
		pass.swapchain.depth_format		  = SG_PIXELFORMAT_NONE;
		if (d3d11.getSampleCount() > 1) {
			pass.swapchain.d3d11.render_view  = d3d11.getMSAARenderTargetView();
			pass.swapchain.d3d11.resolve_view = d3d11.getRenderTargetView();
		} else {
			pass.swapchain.d3d11.render_view = d3d11.getRenderTargetView();
		}
		pass.swapchain.d3d11.depth_stencil_view = d3d11.getDepthStencilView();
		return pass;
	}
} // namespace

// Needs access to handleMessage (friend of MzglVST3View).
struct MzglVST3ViewWin32 {
	static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		if (msg == WM_NCCREATE) {
			auto *cs = reinterpret_cast<CREATESTRUCTW *>(lParam);
			SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
		}
		auto *view = reinterpret_cast<MzglVST3View *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
		if (view != nullptr) {
			bool handled   = false;
			LRESULT result = view->handleMessage(msg, wParam, lParam, handled);
			if (handled) return result;
		}
		return DefWindowProcW(hwnd, msg, wParam, lParam);
	}

	static bool registerClass() {
		static const bool registered = [] {
			WNDCLASSEXW wc {};
			wc.cbSize		 = sizeof(wc);
			wc.style		 = CS_OWNDC | CS_DBLCLKS;
			wc.lpfnWndProc	 = wndProc;
			wc.hInstance	 = dllInstance();
			wc.hCursor		 = LoadCursorW(nullptr, IDC_ARROW);
			wc.lpszClassName = kWindowClassName;
			if (RegisterClassExW(&wc) != 0) return true;
			return GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
		}();
		return registered;
	}
};

struct MzglVST3View::Impl {
	std::shared_ptr<Plugin> plugin;
	std::shared_ptr<PluginEditor> editor;
	std::shared_ptr<Graphics> graphics;
	std::shared_ptr<EventDispatcher> dispatcher;
	std::unique_ptr<D3D11Context> d3d11;
	HWND parent {nullptr};
	HWND hwnd {nullptr};
	bool needsSetup {true};
	bool inFrame {false};

	// Mouse state, as DesktopWindowEventHandler keeps it for the GLFW app.
	float mouseX {0};
	float mouseY {0};
	enum Button { Left = 0, Right = 1, Middle = 2, NumButtons };
	bool buttons[NumButtons] {};

	bool anyButtonDown() const {
		for (bool b: buttons) {
			if (b) return true;
		}
		return false;
	}

	static int touchIdForButton(Button b) {
		switch (b) {
			case Right: return RightMouseButton;
			case Middle: return MiddleMouseButton;
			default: return 0;
		}
	}

	void teardown() {
		if (hwnd != nullptr) {
			KillTimer(hwnd, kFrameTimerId);
		}
		if (editor) {
			editor->pluginViewDisappeared();
		}
		// The editor's layer tree owns sokol resources: destroy it while sokol
		// (never shut down, see SharedGpu) and the device are still around.
		dispatcher.reset();
		editor.reset();
		graphics.reset();
		plugin.reset();
		if (d3d11) {
			d3d11->shutdown();
			d3d11.reset();
		}
		if (hwnd != nullptr) {
			SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
			DestroyWindow(hwnd);
			hwnd = nullptr;
		}
		parent = nullptr;
	}
};

MzglVST3View::MzglVST3View(MzglVST3PluginProvider *provider, const ViewConfig &_config)
	: impl(std::make_unique<Impl>())
	, provider(provider)
	, config(_config) {
	viewRect = {0, 0, config.defaultWidth, config.defaultHeight};
}

MzglVST3View::~MzglVST3View() {
	if (impl) impl->teardown();
}

tresult PLUGIN_API MzglVST3View::queryInterface(const TUID iid, void **obj) {
	QUERY_INTERFACE(iid, obj, FUnknown::iid, IPlugView)
	QUERY_INTERFACE(iid, obj, IPlugView::iid, IPlugView)
	QUERY_INTERFACE(iid, obj, IPlugViewContentScaleSupport::iid, IPlugViewContentScaleSupport)
	*obj = nullptr;
	return kNoInterface;
}

tresult PLUGIN_API MzglVST3View::isPlatformTypeSupported(FIDString type) {
	if (type && strcmp(type, kPlatformTypeHWND) == 0) {
		return kResultTrue;
	}
	return kResultFalse;
}

ViewRect MzglVST3View::scaledRect(int logicalW, int logicalH) const {
	return {0, 0, (int32) (logicalW * contentScale + 0.5f), (int32) (logicalH * contentScale + 0.5f)};
}

tresult PLUGIN_API MzglVST3View::attached(void *parent, FIDString type) {
	if (!parent || !type) return kInvalidArgument;
	if (strcmp(type, kPlatformTypeHWND) != 0) return kResultFalse;
	if (!MzglVST3ViewWin32::registerClass()) return kResultFalse;

	overrideDataPathToBundleResources();

	impl->parent = static_cast<HWND>(parent);

	// A host that never told us its scale: size ourselves for the monitor the
	// parent is on, and let it know below once the window exists.
	bool sizeChanged = false;
	if (!hostSetContentScale) {
		const float scale = dpiScaleForWindow(impl->parent);
		if (scale != contentScale) {
			contentScale = scale;
			viewRect	 = scaledRect(config.defaultWidth, config.defaultHeight);
			sizeChanged	 = true;
		}
	}
	const int width	 = viewRect.getWidth();
	const int height = viewRect.getHeight();

	impl->hwnd = CreateWindowExW(0,
								 kWindowClassName,
								 L"",
								 WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
								 0,
								 0,
								 width,
								 height,
								 impl->parent,
								 nullptr,
								 dllInstance(),
								 this);
	if (impl->hwnd == nullptr) {
		Log::e() << "MzglVST3View: CreateWindowExW failed: " << GetLastError();
		impl->parent = nullptr;
		return kResultFalse;
	}

	// GPU first: an editor may create sokol resources (Vbos for its SVGs,
	// say) as soon as it is constructed, not only from setup().
	auto &gpu	= sharedGpu();
	impl->d3d11 = std::make_unique<D3D11Context>();
	if (!impl->d3d11->init(impl->hwnd, width, height, mzglSokolSampleCount, gpu.device)) {
		Log::e() << "MzglVST3View: D3D11 init failed";
		impl->teardown();
		return kResultFalse;
	}
	if (gpu.device == nullptr) {
		gpu.device = impl->d3d11->getDevice();
		gpu.device->AddRef();

		sg_environment env		  = {};
		env.defaults.sample_count = impl->d3d11->getSampleCount();
		env.defaults.color_format = SG_PIXELFORMAT_BGRA8;
		env.defaults.depth_format = SG_PIXELFORMAT_NONE;
		env.d3d11.device		  = impl->d3d11->getDevice();
		env.d3d11.device_context  = impl->d3d11->getDeviceContext();
		mzglSokolSetup(env);
	}

	// Borrow the controller's plugin so user knob movements and host
	// parameter automation can flow through the same synth instance.
	impl->plugin			   = provider ? provider->getPlugin() : instantiatePlugin();
	impl->graphics			   = std::make_shared<Graphics>();
	impl->graphics->width	   = width;
	impl->graphics->height	   = height;
	impl->graphics->pixelScale = contentScale;
	impl->editor			   = instantiatePluginEditor(*impl->graphics, impl->plugin);
	impl->editor->nativeWindowHandle = impl->hwnd;
	impl->dispatcher				 = std::make_shared<EventDispatcher>(impl->editor);

	impl->needsSetup = true;
	impl->dispatcher->resized();
	renderFrame();
	SetTimer(impl->hwnd, kFrameTimerId, kFrameIntervalMs, nullptr);

	if (sizeChanged && plugFrame != nullptr) {
		plugFrame->resizeView(this, &viewRect);
	}
	return kResultTrue;
}

tresult PLUGIN_API MzglVST3View::removed() {
	impl->teardown();
	return kResultTrue;
}

void MzglVST3View::renderFrame() {
	if (!impl->d3d11 || !impl->dispatcher || impl->inFrame) return;
	impl->inFrame = true;

	sg_begin_pass(makePass(*impl->d3d11, impl->graphics->width, impl->graphics->height));
	const bool firstFrame = impl->needsSetup;
	if (firstFrame) {
		// Same first-draw sequence as MZMetalView: mzgl's main-thread id and
		// timers, then the app's setup(), all with a pass open.
		initMZGL(impl->editor);
		impl->dispatcher->setup();
		impl->needsSetup = false;
	}
	impl->dispatcher->runFrame();
	sg_end_pass();
	sg_commit();
	// No vsync: Present(1) would block the host's UI thread for up to a frame.
	impl->d3d11->present(false);

	impl->inFrame = false;
	if (firstFrame) {
		impl->editor->pluginViewAppeared();
	}
}

std::intptr_t MzglVST3View::handleMessage(unsigned msg, std::uintptr_t wParam, std::intptr_t lParam, bool &handled) {
	auto &I = *impl;
	handled = true;
	switch (msg) {
		case WM_TIMER:
			if (wParam != kFrameTimerId) break;
			renderFrame();
			// Keyboard focus is only ours while a text field is being edited
			// (see WM_LBUTTONDOWN); hand it back so host shortcuts keep working.
			if (I.graphics && I.graphics->textInputReceiver == nullptr && GetFocus() == I.hwnd) {
				SetFocus(I.parent);
			}
			return 0;

		case WM_PAINT: {
			ValidateRect(I.hwnd, nullptr);
			renderFrame();
			return 0;
		}
		case WM_ERASEBKGND: return 1;

		case WM_SIZE: {
			const int w = LOWORD(lParam);
			const int h = HIWORD(lParam);
			if (w > 0 && h > 0 && I.d3d11 && I.graphics && !I.inFrame) {
				I.d3d11->resize(w, h);
				I.graphics->width  = w;
				I.graphics->height = h;
				if (I.dispatcher) I.dispatcher->resized();
			}
			return 0;
		}

		case WM_MOUSEMOVE: {
			if (!I.dispatcher) return 0;
			I.mouseX = (float) GET_X_LPARAM(lParam);
			I.mouseY = (float) GET_Y_LPARAM(lParam);
			if (!I.anyButtonDown()) {
				I.dispatcher->touchOver(I.mouseX, I.mouseY);
			} else {
				for (int b = 0; b < Impl::NumButtons; b++) {
					if (I.buttons[b]) {
						I.dispatcher->touchMoved(I.mouseX, I.mouseY, Impl::touchIdForButton((Impl::Button) b));
					}
				}
			}
			return 0;
		}

		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONDBLCLK: {
			if (!I.dispatcher) return 0;
			const Impl::Button b = (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) ? Impl::Left
								   : (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK) ? Impl::Right
																						: Impl::Middle;
			I.mouseX = (float) GET_X_LPARAM(lParam);
			I.mouseY = (float) GET_Y_LPARAM(lParam);
			if (!I.anyButtonDown()) SetCapture(I.hwnd);
			I.buttons[b] = true;
			I.dispatcher->touchDown(I.mouseX, I.mouseY, Impl::touchIdForButton(b));
			// Like the macOS view we leave keyboard focus with the host (it
			// owns the global shortcuts) - except while a text field is active,
			// which the click may just have focused.
			if (I.graphics && I.graphics->textInputReceiver != nullptr) {
				SetFocus(I.hwnd);
			}
			return 0;
		}

		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP: {
			if (!I.dispatcher) return 0;
			const Impl::Button b = msg == WM_LBUTTONUP ? Impl::Left : msg == WM_RBUTTONUP ? Impl::Right : Impl::Middle;
			I.mouseX = (float) GET_X_LPARAM(lParam);
			I.mouseY = (float) GET_Y_LPARAM(lParam);
			// Only report an up for a down we saw: the button may have gone
			// down outside our window (DesktopWindowEventHandler does the same).
			if (I.anyButtonDown()) {
				I.dispatcher->touchUp(I.mouseX, I.mouseY, Impl::touchIdForButton(b));
			}
			I.buttons[b] = false;
			if (!I.anyButtonDown() && GetCapture() == I.hwnd) ReleaseCapture();
			return 0;
		}

		case WM_MOUSEWHEEL:
		case WM_MOUSEHWHEEL: {
			if (!I.dispatcher) return 0;
			// Notches, same units GLFW reports (and it negates the horizontal
			// axis); then DesktopWindowEventHandler's Windows scroll gain, alt
			// = zoom, shift = swap axes.
			constexpr float scrollSpeed = 5.f;
			const float notches			= (float) GET_WHEEL_DELTA_WPARAM(wParam) / (float) WHEEL_DELTA;
			float dx					= msg == WM_MOUSEHWHEEL ? -notches : 0.f;
			float dy					= msg == WM_MOUSEWHEEL ? notches : 0.f;
			if (GetKeyState(VK_MENU) < 0) {
				I.dispatcher->mouseZoomed(I.mouseX, I.mouseY, dy * -0.03f);
				return 0;
			}
			dx *= scrollSpeed;
			dy *= scrollSpeed;
			if (GetKeyState(VK_SHIFT) < 0) std::swap(dx, dy);
			I.dispatcher->mouseScrolled(I.mouseX, I.mouseY, dx, dy);
			return 0;
		}

		case WM_KEYDOWN:
		case WM_SYSKEYDOWN: {
			if (!I.dispatcher || !I.graphics) return 0;
			// Editing keys for the focused text field; printable characters
			// arrive as WM_CHAR.
			if (I.graphics->textInputReceiver != nullptr) {
				if (wParam == VK_BACK || wParam == VK_DELETE) {
					I.dispatcher->textBackspace();
					return 0;
				}
				if (wParam == VK_RETURN) {
					I.dispatcher->textDone();
					return 0;
				}
				if (wParam == VK_ESCAPE) {
					I.graphics->hideKeyboard();
					return 0;
				}
			}
			if (wParam == VK_TAB && GetKeyState(VK_SHIFT) < 0) {
				I.dispatcher->keyDown(MZ_KEY_SHIFT_TAB);
				return 0;
			}
			I.dispatcher->keyDown(vkToMzglKey(wParam));
			return 0;
		}
		case WM_KEYUP:
		case WM_SYSKEYUP: {
			if (!I.dispatcher) return 0;
			I.dispatcher->keyUp(vkToMzglKey(wParam));
			return 0;
		}
		case WM_CHAR: {
			if (!I.dispatcher || !I.graphics || I.graphics->textInputReceiver == nullptr) return 0;
			static wchar_t highSurrogate = 0;
			const auto ch				 = (wchar_t) wParam;
			if (ch < 32 || ch == 127) return 0; // control characters come via WM_KEYDOWN
			std::wstring w;
			if (IS_HIGH_SURROGATE(ch)) {
				highSurrogate = ch;
				return 0;
			}
			if (IS_LOW_SURROGATE(ch) && highSurrogate != 0) {
				w			  = {highSurrogate, ch};
				highSurrogate = 0;
			} else {
				w = {ch};
			}
			I.dispatcher->textInput(utf16ToUtf8(w));
			return 0;
		}

		case WM_GETDLGCODE: return DLGC_WANTALLKEYS;

		default: break;
	}
	handled = false;
	return 0;
}

tresult PLUGIN_API MzglVST3View::getSize(ViewRect *size) {
	if (!size) return kInvalidArgument;
	*size = viewRect;
	return kResultTrue;
}

tresult PLUGIN_API MzglVST3View::onSize(ViewRect *newSize) {
	if (!newSize) return kInvalidArgument;
	viewRect = *newSize;
	if (impl->hwnd != nullptr) {
		// WM_SIZE does the swap chain / Graphics / resized() work.
		SetWindowPos(impl->hwnd,
					 nullptr,
					 0,
					 0,
					 viewRect.getWidth(),
					 viewRect.getHeight(),
					 SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
	}
	return kResultTrue;
}

tresult PLUGIN_API MzglVST3View::setFrame(IPlugFrame *frame) {
	plugFrame = frame;
	return kResultTrue;
}

tresult PLUGIN_API MzglVST3View::checkSizeConstraint(ViewRect *rect) {
	if (!rect) return kInvalidArgument;
	int width  = rect->right - rect->left;
	int height = rect->bottom - rect->top;

	const ViewRect minRect = scaledRect(config.minWidth, config.minHeight);
	const ViewRect maxRect = scaledRect(config.maxWidth, config.maxHeight);
	if (width < minRect.getWidth()) width = minRect.getWidth();
	if (width > maxRect.getWidth()) width = maxRect.getWidth();
	if (height < minRect.getHeight()) height = minRect.getHeight();
	if (height > maxRect.getHeight()) height = maxRect.getHeight();

	rect->right	 = rect->left + width;
	rect->bottom = rect->top + height;
	return kResultTrue;
}

tresult PLUGIN_API MzglVST3View::setContentScaleFactor(ScaleFactor factor) {
	if (factor <= 0.f) return kInvalidArgument;
	const float oldScale = contentScale;
	contentScale		 = factor;
	hostSetContentScale	 = true;
	if (impl->hwnd == nullptr) {
		// Before attached(): just our preferred size, which getSize() reports.
		viewRect = scaledRect(config.defaultWidth, config.defaultHeight);
		return kResultTrue;
	}
	// Live scale change (window dragged to another monitor): redraw at the new
	// scale and ask the host for the equivalent size.
	impl->graphics->pixelScale = contentScale;
	if (impl->dispatcher) impl->dispatcher->resized();
	if (plugFrame != nullptr && oldScale > 0.f) {
		ViewRect wanted = scaledRect((int) (viewRect.getWidth() / oldScale + 0.5f),
									 (int) (viewRect.getHeight() / oldScale + 0.5f));
		plugFrame->resizeView(this, &wanted);
	}
	return kResultTrue;
}

} // namespace mzglvst

#endif // _WIN32

#include "Win32Touch.h"

#include <windows.h>
#include <commctrl.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include "glfw3.h"
#include <GLFW/glfw3native.h>

#include <algorithm>
#include <map>

// mzgl builds for _WIN32_WINNT 0x0601 (Windows 7) and the pointer API is Windows 8+,
// so <windows.h> may not declare it (and the PCH may already have included it at
// 0x0601). Declare the few bits used here under our own names, and resolve the
// functions at runtime so the exe still loads where they don't exist.
namespace {
	constexpr UINT wmPointerUpdate		   = 0x0245;
	constexpr UINT wmPointerDown		   = 0x0246;
	constexpr UINT wmPointerUp			   = 0x0247;
	constexpr UINT wmPointerCaptureChanged = 0x024C;
	constexpr DWORD pointerFlagInContact   = 0x0004; // POINTER_MESSAGE_FLAG_INCONTACT
	constexpr DWORD pointerTypeTouch	   = 2; // PT_TOUCH

	// FEEDBACK_TYPE values: the touch visuals Windows draws for its own gestures
	constexpr int feedbackContactVisualization = 1;
	constexpr int feedbackTap				   = 4;
	constexpr int feedbackDoubleTap			   = 6;
	constexpr int feedbackPressAndHold		   = 7;
	constexpr int feedbackRightTap			   = 8;
	constexpr int feedbackPressAndTap		   = 11;

	using GetPointerTypeFn = BOOL(WINAPI *)(UINT32 pointerId, DWORD *pointerType);
	using SetWindowFeedbackSettingFn =
		BOOL(WINAPI *)(HWND, int feedback, DWORD flags, UINT32 size, const void *value);

	constexpr UINT_PTR subclassId = 0x4D5A5443; // 'MZTC'

	UINT32 pointerId(WPARAM wParam) {
		return LOWORD(wParam);
	}
	bool inContact(WPARAM wParam) {
		return (HIWORD(wParam) & pointerFlagInContact) != 0;
	}

	LRESULT CALLBACK subclassProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
} // namespace

struct Win32Touch::Impl {
	HWND hwnd {nullptr};
	Callback callback;
	GetPointerTypeFn getPointerType {nullptr};

	struct Contact {
		int slot;
		double x;
		double y;
	};
	// keyed by the Windows pointer id; only touches that went down in our client area
	std::map<UINT32, Contact> active;

	~Impl() {
		// no callbacks from here: the app may already be gone
		if (hwnd != nullptr) RemoveWindowSubclass(hwnd, subclassProc, subclassId);
	}

	bool isTouch(UINT32 id) const {
		DWORD type = 0;
		return getPointerType(id, &type) && type == pointerTypeTouch;
	}

	int freeSlot() const {
		int slot = 0;
		while (std::any_of(active.begin(), active.end(), [slot](auto &e) { return e.second.slot == slot; })) {
			slot++;
		}
		return slot;
	}

	void down(UINT32 id, double x, double y) {
		if (active.count(id) != 0) return;
		const int slot = freeSlot();
		active[id]	   = {slot, x, y};
		callback(Phase::down, slot, x, y);
	}

	void moved(UINT32 id, double x, double y) {
		auto it = active.find(id);
		if (it == active.end()) return;
		auto &c = it->second;
		if (c.x == x && c.y == y) return; // pressure/size-only updates
		c.x = x;
		c.y = y;
		callback(Phase::moved, c.slot, x, y);
	}

	void up(UINT32 id, double x, double y) {
		auto it = active.find(id);
		if (it == active.end()) return;
		const int slot = it->second.slot;
		active.erase(it);
		callback(Phase::up, slot, x, y);
	}

	// mzgl has no touch-cancel event, so end the touch where it last was
	void cancel(UINT32 id) {
		auto it = active.find(id);
		if (it == active.end()) return;
		up(id, it->second.x, it->second.y);
	}
};

namespace {
	LRESULT CALLBACK subclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR refData) {
		auto *impl = reinterpret_cast<Win32Touch::Impl *>(refData);
		switch (msg) {
			case wmPointerDown:
			case wmPointerUpdate:
			case wmPointerUp: {
				const UINT32 id = pointerId(wParam);
				if (!impl->isTouch(id)) break; // mouse / pen: leave it to GLFW

				// lParam is in screen coordinates
				POINT p {static_cast<short>(LOWORD(lParam)), static_cast<short>(HIWORD(lParam))};
				ScreenToClient(hwnd, &p);
				const double x = p.x;
				const double y = p.y;

				if (msg == wmPointerDown) {
					impl->down(id, x, y);
				} else if (msg == wmPointerUp) {
					impl->up(id, x, y);
				} else if (inContact(wParam)) {
					impl->moved(id, x, y);
				}
				// handled: stops DefWindowProc promoting the touch to mouse
				// messages, which is where the press-and-hold right-click and the
				// tap/hold disambiguation delay come from
				return 0;
			}

			case wmPointerCaptureChanged: impl->cancel(pointerId(wParam)); break;

			case WM_NCDESTROY:
				RemoveWindowSubclass(hwnd, subclassProc, subclassId);
				impl->hwnd = nullptr;
				impl->active.clear();
				break;
		}
		return DefSubclassProc(hwnd, msg, wParam, lParam);
	}
} // namespace

auto Win32Touch::create(GLFWwindow *window, Callback callback) -> std::unique_ptr<Win32Touch> {
	HWND hwnd = glfwGetWin32Window(window);
	if (hwnd == nullptr) return nullptr;

	HMODULE user32 = GetModuleHandleW(L"user32.dll");
	if (user32 == nullptr) return nullptr;
	auto getPointerType = reinterpret_cast<GetPointerTypeFn>(GetProcAddress(user32, "GetPointerType"));
	if (getPointerType == nullptr) return nullptr; // pre-Windows 8

	auto impl			 = std::make_unique<Impl>();
	impl->hwnd			 = hwnd;
	impl->callback		 = std::move(callback);
	impl->getPointerType = getPointerType;

	if (!SetWindowSubclass(hwnd, subclassProc, subclassId, reinterpret_cast<DWORD_PTR>(impl.get()))) {
		impl->hwnd = nullptr;
		return nullptr;
	}

	// The touch visuals (contact ripple, press-and-hold ring) belong to the gestures
	// we no longer use; an instrument shouldn't draw them under the fingers.
	if (auto setFeedback =
			reinterpret_cast<SetWindowFeedbackSettingFn>(GetProcAddress(user32, "SetWindowFeedbackSetting"))) {
		const BOOL off = FALSE;
		for (int f: {feedbackContactVisualization,
					 feedbackTap,
					 feedbackDoubleTap,
					 feedbackPressAndHold,
					 feedbackRightTap,
					 feedbackPressAndTap}) {
			setFeedback(hwnd, f, 0, sizeof(off), &off);
		}
	}

	return std::unique_ptr<Win32Touch>(new Win32Touch(std::move(impl)));
}

Win32Touch::Win32Touch(std::unique_ptr<Impl> impl)
	: impl(std::move(impl)) {
}

Win32Touch::~Win32Touch() = default;

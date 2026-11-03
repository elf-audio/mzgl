#pragma once

#include "NativeTouch.h"
#include <memory>

struct GLFWwindow;

// Windows turns touch into emulated mouse input unless the window takes the
// WM_POINTER messages itself. That emulation has to tell a tap from a
// press-and-hold (which it turns into a right-click), so the button-down only
// lands on release or after the hold timeout, and it carries a single contact.
// GLFW leaves WM_POINTER to DefWindowProc, so this subclasses GLFW's HWND and
// handles the touch pointers itself: each finger is reported as it lands, with
// its own id (the lowest free slot from 0, as on iOS). Mouse and pen input still
// go through GLFW untouched. Messages arrive via glfwPollEvents()/glfwWaitEvents(),
// so the callback runs on the main thread like the GLFW input callbacks.
class Win32Touch {
public:
	using Phase = NativeTouch::Phase;

	// x, y are client-area (window) coordinates, like GLFW's cursor position
	using Callback = NativeTouch::Callback;

	// Returns nullptr if the window has no HWND or the system has no pointer API
	// (pre-Windows 8), in which case touch stays mouse-emulated as before.
	static auto create(GLFWwindow *window, Callback callback) -> std::unique_ptr<Win32Touch>;

	// Same, for a native HWND that isn't a GLFW window (the VST3 editor's child
	// window). Messages arrive through that window's own message loop - in a
	// plugin, the host's UI thread - so the callback runs there.
	static auto createForHwnd(void *hwnd, Callback callback) -> std::unique_ptr<Win32Touch>;

	~Win32Touch();

	struct Impl;

private:
	explicit Win32Touch(std::unique_ptr<Impl> impl);
	std::unique_ptr<Impl> impl;
};

#include "OS.h"
#include <glew.h>
#include "glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_NATIVE_INCLUDE_NONE
#include <glfw/glfw3native.h>

namespace os {

struct WindowContextFunction {
	std::function<void()> fn;
};

static auto CALLBACK windowContextCallback(HWND hwnd, UINT uMsg, ULONG_PTR dwData, LRESULT lResult) -> void {
	const auto fn{reinterpret_cast<WindowContextFunction*>(dwData)};
	fn->fn();
	delete fn;
	lResult = 0;
}

auto executeInWindowContext(void* nativeWindowHandle, std::function<void()> fn) -> void {
	const auto hwnd{reinterpret_cast<HWND>(nativeWindowHandle)};
	const auto contextFn{new WindowContextFunction{std::move(fn)}};
	const auto dwData{reinterpret_cast<UINT_PTR>(contextFn)};
	SendMessageCallback(hwnd, WM_USER, 0, 0, windowContextCallback, dwData);
}

auto getNativeWindowHandle(GLFWwindow* window) -> void* {
	return glfwGetWin32Window(window);
}

auto initializeGLContext() -> void {
    glewInit();
    glEnable(GL_MULTISAMPLE);
}

auto setWindowSize(GLFWwindow* window, int width, int height) -> void {
	const auto hwnd{glfwGetWin32Window(window)};
	SetWindowPos(hwnd, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

auto attachWindow(GLFWwindow* window, void* nativeParentWindowHandle) -> void {
	const auto hwnd{glfwGetWin32Window(window)};
	const auto parent{reinterpret_cast<HWND>(nativeParentWindowHandle)};
	SetParent(hwnd, parent);
	SetWindowLongPtr(hwnd, GWL_STYLE, WS_CHILD);
	RECT parentRect;
	POINT childPos;
	SIZE childSize;
	GetClientRect(parent, &parentRect);
	childPos.x = parentRect.left;
	childPos.y = parentRect.top;
	childSize.cx = parentRect.right - parentRect.left;
	childSize.cy = parentRect.bottom - parentRect.top;
	SetWindowPos(hwnd, NULL, childPos.x, childPos.y, childSize.cx, childSize.cy, SWP_NOZORDER | SWP_NOACTIVATE);
}

auto detachWindow(GLFWwindow* window) -> void {
	const auto hwnd{glfwGetWin32Window(window)};
	SetParent(hwnd, nullptr);
}

} // os

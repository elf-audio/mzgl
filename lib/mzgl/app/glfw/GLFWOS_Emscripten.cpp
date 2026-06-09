#include "GLFWOS.h"

// Emscripten/WebGL has no native window handle and no plugin host parenting.
// These are all no-ops; the browser canvas is the only surface.

namespace os {

	auto getNativeWindowHandle(GLFWwindow *window) -> void * {
		return nullptr;
	}

	auto initializeGLContext() -> void {
		// WebGL context is created by GLFW/emscripten; nothing to do.
	}

	auto executeInWindowContext(void *nativeWindowHandle, std::function<void()> fn) -> void {
		if (fn) fn();
	}

	auto attachWindow(GLFWwindow *window, void *nativeParentWindowHandle) -> void {}
	auto detachWindow(GLFWwindow *window) -> void {}
	auto setWindowSize(GLFWwindow *window, int width, int height) -> void {}

} // namespace os

#pragma once

#include <functional>

struct GLFWwindow;

namespace os {

// Execute the function in the target window's thread. Should return immediately.
auto executeInWindowContext(void* nativeWindowHandle, std::function<void()> fn) -> void;

// Return the native window handle for the GLFW window.
auto getNativeWindowHandle(GLFWwindow* window) -> void*;

// Call this after the GLFW context is made current for the first time.
auto initializeGLContext() -> void;

// Attach the GLFW window to the parent window (used for VST plugin)
auto attachWindow(GLFWwindow* window, void* nativeParentWindowHandle) -> void;

// Detach the GLFW window from the parent window (used for VST plugin)
auto detachWindow(GLFWwindow* window) -> void;

// Use this to set the window size. (Use this instead of glfwSetWindowSize if
// the window is set as a child of a parent window (e.g. in the VST version).
// In this situation gflwSetWindowSize doesn't do the right thing.
auto setWindowSize(GLFWwindow* window, int width, int height) -> void;

} // os
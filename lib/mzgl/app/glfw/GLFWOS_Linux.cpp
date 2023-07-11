#include "GLFWOS.h"
#include <glew.h>
#include "glfw3.h"
#include <glfw/glfw3native.h>

namespace os {

auto getNativeWindowHandle(GLFWwindow* window) -> void* {
	return glfwGetGLXWindow(window);
}

auto initializeGLContext() -> void {
	// Linux doesn't need to do anything specific right now?
}

/*

If we build a VST version for Linux then these will need to be implemented:

auto setWindowSize(GLFWwindow* window, int width, int height) -> void {
}

auto attachWindow(GLFWwindow* window, void* nativeParentWindowHandle) -> void {
}

auto detachWindow(GLFWwindow* window) -> void {
}

*/

} // os

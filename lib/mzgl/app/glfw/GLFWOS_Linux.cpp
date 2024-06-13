#define GLFW_EXPOSE_NATIVE_GLX
#define GLFW_NATIVE_INCLUDE_NONE

#include "GLFWOS.h"
#include <glew/include/GL/glew.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

namespace os {

	auto getNativeWindowHandle(GLFWwindow *window) -> void * {
		return (void *) (glfwGetGLXWindow(window)); // FIXME: don't like that cast, it smells
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

} // namespace os

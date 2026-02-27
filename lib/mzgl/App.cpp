#include "App.h"
#include "mzgl_platform.h"

#if MZGL_MAC
#	include <Cocoa/Cocoa.h>
#endif

#if MZGL_WIN || MZGL_LINUX
struct GLFWwindow;
extern "C" void glfwSetWindowTitle(GLFWwindow *window, const char *title);
#endif

void App::setWindowTitle(const std::string &title) {
#if MZGL_MAC
	if (windowHandle == nullptr) return;
	NSWindow *win = (__bridge NSWindow *) windowHandle;
	std::string titleCopy = title;
	dispatch_async(dispatch_get_main_queue(), ^(void) {
	  [win setTitle:[NSString stringWithUTF8String:titleCopy.c_str()]];
	});
#elif MZGL_WIN || MZGL_LINUX
	if (windowHandle == nullptr) return;
	glfwSetWindowTitle(static_cast<GLFWwindow *>(windowHandle), title.c_str());
#endif
}

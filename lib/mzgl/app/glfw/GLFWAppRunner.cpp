//
//  main.m
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#ifdef _WIN32 ////////////////////////////////////////////////

// This enables visual styles on Windows
#	pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#	include <glew.h>
#endif // _WIN32 /////////////////////////////////////////////

#ifdef __linux__ /////////////////////////////////////////////
#	include <gtk/gtk.h>
#endif // __linux__ //////////////////////////////////////////

#include "GLFWAppRunner.h"
#include "glfw3.h"
#include <optional>
#include <stdlib.h>
#include <stdio.h>
#include "filesystem.h"
#include "DesktopWindowEventHandler.h"
#include "DesktopWindowFileDragHandler.h"
#include "util.h"
#include "log.h"
#include "GLFWOS.h"

using namespace std;

DesktopWindowEventHandler windowEventHandler;
bool framebuferResized = false;

EventDispatcher *getEventDispatcher(GLFWwindow *window) {
	auto *app = (GLFWAppRunner *) glfwGetWindowUserPointer(window);
	return app->eventDispatcher.get();
}

Graphics &getGraphics(GLFWwindow *window) {
	auto *app = (GLFWAppRunner *) glfwGetWindowUserPointer(window);
	return app->graphics;
}

static void error_callback(int error, const char *description) {
	// fprintf(stderr, "Error: %s\n", description);
	Log::e() << "GLFW Error: " << description;
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
	windowEventHandler.key(getEventDispatcher(window), key, action);
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
	windowEventHandler.mouseButton(getEventDispatcher(window), button, action);
}

void cursor_position_callback(GLFWwindow *window, double xpos, double ypos) {
	windowEventHandler.cursorPos(getEventDispatcher(window), static_cast<float>(xpos), static_cast<float>(ypos));
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
	windowEventHandler.scroll(getEventDispatcher(window), xoffset, yoffset);
}

void window_size_callback(GLFWwindow *window, int width, int height) {
	auto &g = getGraphics(window);
	glViewport(0, 0, width, height);

	g.width	 = width;
	g.height = height;

	framebuferResized = true; // just mark that we need to resize
	Log::d() << "resized to " << width << height;
}

float getMainMonitorScale() {
	GLFWmonitor *monitor = glfwGetPrimaryMonitor();
	if (monitor != nullptr) {
		float xscale, yscale;
		glfwGetMonitorContentScale(monitor, &xscale, &yscale);
		Log::d() << "scale x: " << xscale << " scale y: " << yscale;
		return xscale;
	} else {
		Log::d() << "Can't find main monitor";
	}
	return 1;
}

void GLFWAppRunner::setCallbacks() {
	if (window == nullptr) {
		printf("ERROR! window not initialized yet!\n");
	}
	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetScrollCallback(window, scroll_callback);
	//glfwSetWindowSizeCallback(window, window_size_callback);
	glfwSetFramebufferSizeCallback(window, window_size_callback);
	glfwSetWindowUserPointer(window, this);
}

#ifdef METAL_BACKEND
// lifting from here: https://gist.github.com/gcatlin/987be74e2d58da96093a7598f3fbfb27
#	import <Metal/Metal.h>
#	import <QuartzCore/CAMetalLayer.h>
#endif

void GLFWAppRunner::run(int argc, char *argv[]) {
	loadCommandLineArgs(argc, (const char **) argv);

	glfwSetErrorCallback(error_callback);
	if (!glfwInit()) {
		printf("Can't init GLFW\n");
		exit(EXIT_FAILURE);
	}

	graphics.pixelScale = 1.0f; // actually we don't need any scaling, this is legacy code for compatibility

	glfwWindowHint(GLFW_SAMPLES, 16);

#ifdef METAL_BACKEND

	const id<MTLDevice> gpu			= MTLCreateSystemDefaultDevice();
	const id<MTLCommandQueue> queue = [gpu newCommandQueue];
	CAMetalLayer *swapchain			= [CAMetalLayer layer];
	swapchain.device				= gpu;
	swapchain.opaque				= YES;

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

#else

#	ifdef UNIT_TEST
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#	else
	// this was set to 2.0 before, I bumped it to 3.2 so I can use imgui
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#	endif
#endif

	// this is crashing on mac for some reason
	//	const unsigned char *openglVersion = glGetString(GL_VERSION);
	//	if (openglVersion != nullptr) {
	//		Log::d() << "GLFWAppRunner - OpenGL Version: " << openglVersion;
	//	} else {
	//		Log::d() << "OpenGL Version: unknown";
	//	}
	int requestedWidth	= -1;
	int requestedHeight = -1;

	GLFWmonitor *primaryMonitor = glfwGetPrimaryMonitor();
	if (primaryMonitor != nullptr) {
		const GLFWvidmode *currentVideoMode = glfwGetVideoMode(primaryMonitor);
		if (currentVideoMode != nullptr) {
			float h =
				(currentVideoMode->height) * 0.8; // make it 0.9 of max height, so there is room for decorations
			float w			= h * 0.54; // set width as 0.54 of height, that looks OK
			requestedHeight = (int) h;
			requestedWidth	= (int) w;
			if (requestedWidth > currentVideoMode->width) {
				requestedWidth = currentVideoMode->width; // clamp in case of pivoted monitor
			}
		}
	}

	// Note that graphics object is not fully functional here
	// as we need to update width/height later on.
	app = instantiateApp(graphics);

	if (requestedWidth != -1) {
		graphics.width	= requestedWidth;
		graphics.height = requestedHeight;
	}

	Log::d() << "Request window " << (graphics.width) << "x" << (graphics.height);

	window = glfwCreateWindow(graphics.width, graphics.height, "mzgl", NULL, NULL);
	if (!window) {
		printf("Can't create GLFW window\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
#ifdef METAL_BACKEND
	NSWindow *nswindow				= glfwGetCocoaWindow(window);
	nswindow.contentView.layer		= swapchain;
	nswindow.contentView.wantsLayer = YES;
	MTLClearColor color				= MTLClearColorMake(0, 0, 0, 1);
#endif

	int windowH, windowW;
	glfwGetWindowSize(window, &windowW, &windowH); // note that it can be DIFFERENT than requested
	Log::d() << "Window crated: " << windowW << "x" << windowH;

	// Use Frame Buffer pixel size as graphics context size
	// That would be compatible with GL functions using pixel coords (glViewport, glScissors, etc.)
	glfwGetFramebufferSize(window, &windowW, &windowH);
	Log::d() << "FB size: " << windowW << "x" << windowH;

	graphics.width	= windowW;
	graphics.height = windowH;

	app->windowHandle		= window;
	app->nativeWindowHandle = os::getNativeWindowHandle(window);

	// Create the event dispatcher
	eventDispatcher = std::make_shared<EventDispatcher>(app);

	// Platform-specific file drag/drop handler.
	DesktopWindowFileDragHandler windowFileDragHandler {
		window, file_drag_handler::makeFileDragListener(eventDispatcher.get())};

#ifdef __linux__
	gtk_init(&argc, &argv);
#endif

	if (argc > 0) {
		glfwSetWindowTitle(window, fs::path(argv[0]).filename().string().c_str());
	}

	setCallbacks();

	// NOTE: OpenGL error checks have been omitted for brevity
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	// Platform-specific GL setup
	os::initializeGLContext();

	initMZGL(app);

	eventDispatcher->setup();

	while (!glfwWindowShouldClose(window)) {
#ifdef METAL_BACKEND
		@autoreleasepool {
			color.red = (color.red > 1.0) ? 0 : color.red + 0.01;

			id<CAMetalDrawable> surface = [swapchain nextDrawable];

			MTLRenderPassDescriptor *pass		 = [MTLRenderPassDescriptor renderPassDescriptor];
			pass.colorAttachments[0].clearColor	 = color;
			pass.colorAttachments[0].loadAction	 = MTLLoadActionClear;
			pass.colorAttachments[0].storeAction = MTLStoreActionStore;
			pass.colorAttachments[0].texture	 = surface.texture;

			id<MTLCommandBuffer> buffer			= [queue commandBuffer];
			id<MTLRenderCommandEncoder> encoder = [buffer renderCommandEncoderWithDescriptor:pass];
			[encoder endEncoding];
			[buffer presentDrawable:surface];
			[buffer commit];
		}
#else
		eventDispatcher->runFrame();

		glfwSwapBuffers(window);
		glfwPollEvents();

		// While resizing glfwPollEvents blocks until user finishes action, so we can dispatch event here
		// This is optimization to avoid dispatching from every resize callback, which can be overkill
		if (framebuferResized) {
			getEventDispatcher(window)->resized();
			framebuferResized = false;
		}
#endif
	}

	eventDispatcher->exit();

	glfwDestroyWindow(window);
	glfwTerminate();
}

void GLFWAppRunner::stop() {
	glfwSetWindowShouldClose(window, true);
}

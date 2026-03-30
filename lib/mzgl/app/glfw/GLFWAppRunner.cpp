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

#	ifndef MZGL_SOKOL
#		include <glew.h>
#	endif
#endif // _WIN32 /////////////////////////////////////////////

#ifdef __linux__ /////////////////////////////////////////////
#	include <gtk/gtk.h>
#endif // __linux__ //////////////////////////////////////////

#include "GLFWAppRunner.h"
#include "glfw3.h"
#include <optional>
#include <stdlib.h>
#include <stdio.h>
#include <thread>
#include <chrono>
#include "filesystem.h"
#include "DesktopWindowEventHandler.h"
#include "DesktopWindowFileDragHandler.h"
#include "util.h"
#include "log.h"
#include "GLFWOS.h"

#if defined(MZGL_SOKOL) && defined(_WIN32)
#	include "D3D11Context.h"
#	include "sokol_gfx.h"
#	include "sokol_log.h"
#endif

#if defined(WIN32) || defined(__linux__)
void quitApplication() {
	glfwTerminate();
}
#endif

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

int convertGlfwKeyToMzgl(int key) {
	switch (key) {
		case GLFW_KEY_LEFT: return MZ_KEY_LEFT;
		case GLFW_KEY_RIGHT: return MZ_KEY_RIGHT;
		case GLFW_KEY_DOWN: return MZ_KEY_DOWN;
		case GLFW_KEY_UP: return MZ_KEY_UP;
		case GLFW_KEY_BACKSPACE:
		case GLFW_KEY_DELETE: return MZ_KEY_DELETE;
		case GLFW_KEY_TAB: return MZ_KEY_TAB;
		case GLFW_KEY_ESCAPE: return MZ_KEY_ESCAPE;
		case MZ_KEY_RETURN:
		case GLFW_KEY_ENTER: return MZ_KEY_RETURN;
		case GLFW_KEY_RIGHT_SHIFT:
		case GLFW_KEY_LEFT_SHIFT: return MZ_KEY_SHIFT;
		case GLFW_KEY_RIGHT_CONTROL:
		case GLFW_KEY_LEFT_CONTROL: return MZ_KEY_CTRL;
		case GLFW_KEY_RIGHT_ALT:
		case GLFW_KEY_LEFT_ALT: return MZ_KEY_ALT;
		case GLFW_KEY_LEFT_SUPER:
		case GLFW_KEY_RIGHT_SUPER: return MZ_KEY_CMD;
		case GLFW_KEY_PAGE_UP: return MZ_KEY_INCREMENT;
		case GLFW_KEY_PAGE_DOWN: return MZ_KEY_DECREMENT;
		default: return key;
	}
}
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_TAB && (mods & GLFW_MOD_SHIFT)) {
		windowEventHandler.key(getEventDispatcher(window), MZ_KEY_SHIFT_TAB, action);
		return;
	}
	key = convertGlfwKeyToMzgl(key);
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
#ifndef MZGL_SOKOL
	glViewport(0, 0, width, height);
#endif
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

#ifdef METAL_BACKEND

	const id<MTLDevice> gpu			= MTLCreateSystemDefaultDevice();
	const id<MTLCommandQueue> queue = [gpu newCommandQueue];
	CAMetalLayer *swapchain			= [CAMetalLayer layer];
	swapchain.device				= gpu;
	swapchain.opaque				= YES;

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

#elif defined(MZGL_SOKOL) && defined(_WIN32)
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

#else
	glfwWindowHint(GLFW_SAMPLES, 16);

#	ifdef UNIT_TEST
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#	else

#		ifdef __arm__ // raspberry pi?
	// so we're going for gles
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#		else

	// this was set to 2.0 before, I bumped it to 3.2 so I can use imgui
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
#		endif

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

	Log::d() << "Request window " << (graphics.width) << " x " << (graphics.height);

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

#if defined(MZGL_SOKOL) && defined(_WIN32)
	// D3D11 + Sokol backend for Windows
	D3D11Context d3d11;
	{
		HWND hwnd = (HWND) os::getNativeWindowHandle(window);
		if (!d3d11.init(hwnd, graphics.width, graphics.height)) {
			Log::e() << "Failed to initialize D3D11";
			glfwDestroyWindow(window);
			glfwTerminate();
			exit(EXIT_FAILURE);
		}

		sg_desc desc		  = {};
		desc.environment	  = {};
		desc.logger.func	  = slog_func;
		desc.buffer_pool_size = 4096;
		desc.shader_pool_size = 128;

		desc.environment.defaults.sample_count = d3d11.getSampleCount();
		desc.environment.defaults.color_format = SG_PIXELFORMAT_BGRA8;
		desc.environment.defaults.depth_format = SG_PIXELFORMAT_NONE;

		desc.environment.d3d11.device		  = d3d11.getDevice();
		desc.environment.d3d11.device_context = d3d11.getDeviceContext();

		sg_setup(desc);

		// Force NVIDIA driver to fully initialize by creating and destroying a dummy texture
		uint32_t white = 0xFFFFFFFF;
		sg_image_desc dummyDesc = {};
		dummyDesc.width			= 1;
		dummyDesc.height		= 1;
		dummyDesc.pixel_format	= SG_PIXELFORMAT_RGBA8;
		dummyDesc.data.subimage[0][0] = {&white, sizeof(white)};
		sg_image dummyImg = sg_make_image(dummyDesc);
		sg_destroy_image(dummyImg);
	}

	// Helper to build the sokol swapchain pass struct
	auto makePass = [&]() {
		sg_pass pass						 = {};
		pass.action.colors[0].load_action	 = SG_LOADACTION_CLEAR;
		pass.action.colors[0].clear_value	 = {0.f, 0.f, 0.f, 1.f};
		pass.swapchain.width				 = graphics.width;
		pass.swapchain.height		= graphics.height;
		pass.swapchain.sample_count = d3d11.getSampleCount();
		pass.swapchain.color_format = SG_PIXELFORMAT_BGRA8;
		pass.swapchain.depth_format = SG_PIXELFORMAT_NONE;
		if (d3d11.getSampleCount() > 1) {
			pass.swapchain.d3d11.render_view  = d3d11.getMSAARenderTargetView();
			pass.swapchain.d3d11.resolve_view = d3d11.getRenderTargetView();
		} else {
			pass.swapchain.d3d11.render_view = d3d11.getRenderTargetView();
		}
		pass.swapchain.d3d11.depth_stencil_view = d3d11.getDepthStencilView();
		return pass;
	};

	// Warm up the D3D11 driver with a dummy frame before creating resources
	sg_begin_pass(makePass());
	sg_end_pass();
	sg_commit();
	d3d11.present();

	initMZGL(app);
	bool needsSetup = true;

	while (!glfwWindowShouldClose(window)) {
		if (glfwGetWindowAttrib(window, GLFW_ICONIFIED)) {
			glfwPollEvents();
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			continue;
		}

		if (framebuferResized) {
			d3d11.resize(graphics.width, graphics.height);
			eventDispatcher->resized();
			framebuferResized = false;
		}

		sg_begin_pass(makePass());
		if (needsSetup) {
			eventDispatcher->setup();
			needsSetup = false;
		}
		eventDispatcher->runFrame();
		sg_end_pass();
		sg_commit();

		d3d11.present();

		glfwPollEvents();
	}

	sg_shutdown();
	d3d11.shutdown();

#else
	// NOTE: OpenGL error checks have been omitted for brevity
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	// Platform-specific GL setup
	os::initializeGLContext();

	initMZGL(app);

	eventDispatcher->setup();

	while (!glfwWindowShouldClose(window)) {
#	ifdef METAL_BACKEND
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
#	else
		// When minimized, skip rendering to avoid memory leak and "not responding" state.
		// Just poll events and sleep briefly to keep the app responsive without burning CPU/RAM.
		if (glfwGetWindowAttrib(window, GLFW_ICONIFIED)) {
			glfwPollEvents();
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		} else {
			eventDispatcher->runFrame();

			glfwSwapBuffers(window);

			glfwPollEvents();

			if (framebuferResized) {
				getEventDispatcher(window)->resized();
				framebuferResized = false;
			}
		}
#	endif
	}
#endif

	eventDispatcher->exit();

	glfwDestroyWindow(window);
	glfwTerminate();
}

void GLFWAppRunner::stop() {
	glfwSetWindowShouldClose(window, true);
}

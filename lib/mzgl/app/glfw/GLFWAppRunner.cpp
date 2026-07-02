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
#	include "SokolSetup.h"
#endif

#if defined(MZGL_SOKOL) && defined(__EMSCRIPTEN__)
#	include "sokol_gfx.h"
#	include "SokolSetup.h"
#	include <emscripten/emscripten.h>
#	include <emscripten/html5.h>
#endif

#if defined(_WIN32) || defined(__linux__)
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

#if defined(MZGL_SOKOL) && defined(__EMSCRIPTEN__)
// The browser owns the run loop, so we can't block in a while(). Instead we
// hand a per-frame callback to emscripten_set_main_loop and keep the runner
// state in a static pointer for the (argument-less) C callback to reach.
static GLFWAppRunner *gEmscriptenRunner = nullptr;

static void emscriptenMainLoop() {
	auto *self = gEmscriptenRunner;
	if (self == nullptr) return;

	if (framebuferResized) {
		self->eventDispatcher->resized();
		framebuferResized = false;
	}

	// The emscripten GLFW port keeps resetting the canvas drawing-buffer size,
	// so re-assert it each frame to match our framebuffer.
	{
		int cw = 0, ch = 0;
		emscripten_get_canvas_element_size("#canvas", &cw, &ch);
		if (cw != self->graphics.width || ch != self->graphics.height) {
			emscripten_set_canvas_element_size(
				"#canvas", self->graphics.width, self->graphics.height);
		}
	}

	sg_pass pass				  = {};
	pass.swapchain.width		  = self->graphics.width;
	pass.swapchain.height		  = self->graphics.height;
	pass.swapchain.sample_count	  = mzglSokolSampleCount; // matches sg_setup + GLFW_SAMPLES
	pass.swapchain.gl.framebuffer = 0; // WebGL default framebuffer

	sg_begin_pass(pass);
	self->eventDispatcher->runFrame();
	sg_end_pass();
	sg_commit();

	glfwSwapBuffers(self->window);
	glfwPollEvents();
}
#endif

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
	// If a text field is focused, editing keys go to it (printable characters
	// arrive separately via char_callback).
	if ((action == GLFW_PRESS || action == GLFW_REPEAT) && getGraphics(window).textInputReceiver != nullptr) {
		auto *ed = getEventDispatcher(window);
		if (key == GLFW_KEY_BACKSPACE || key == GLFW_KEY_DELETE) {
			ed->textBackspace();
			return;
		}
		if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) {
			ed->textDone();
			return;
		}
		if (key == GLFW_KEY_ESCAPE) {
			getGraphics(window).hideKeyboard();
			return;
		}
	}
	if (key == GLFW_KEY_TAB && (mods & GLFW_MOD_SHIFT)) {
		windowEventHandler.key(getEventDispatcher(window), MZ_KEY_SHIFT_TAB, action);
		return;
	}
	key = convertGlfwKeyToMzgl(key);
	windowEventHandler.key(getEventDispatcher(window), key, action);
}

// Unicode text entry (respects keyboard layout / IME). Only delivered while a
// text field is focused; printable input only — editing keys come via key_callback.
static void char_callback(GLFWwindow *window, unsigned int codepoint) {
	if (getGraphics(window).textInputReceiver == nullptr) return;
	std::string utf8;
	if (codepoint < 0x80) {
		utf8 += static_cast<char>(codepoint);
	} else if (codepoint < 0x800) {
		utf8 += static_cast<char>(0xC0 | (codepoint >> 6));
		utf8 += static_cast<char>(0x80 | (codepoint & 0x3F));
	} else if (codepoint < 0x10000) {
		utf8 += static_cast<char>(0xE0 | (codepoint >> 12));
		utf8 += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
		utf8 += static_cast<char>(0x80 | (codepoint & 0x3F));
	} else {
		utf8 += static_cast<char>(0xF0 | (codepoint >> 18));
		utf8 += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
		utf8 += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
		utf8 += static_cast<char>(0x80 | (codepoint & 0x3F));
	}
	getEventDispatcher(window)->textInput(utf8);
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
	glfwSetCharCallback(window, char_callback);
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

#elif defined(__EMSCRIPTEN__)
	// WebGL2 == GLES3. Request 4x MSAA on the default framebuffer (the browser
	// resolves it); sokol pipelines + the swapchain pass must agree on the
	// sample count (see sg_setup and emscriptenMainLoop below).
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_SAMPLES, 4); // must match mzglSokolSampleCount (SokolSetup.h)

#else
	glfwWindowHint(GLFW_SAMPLES, 4); // 4x MSAA, matching mzglSokolSampleCount on the sokol backends

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
	// Note that graphics object is not fully functional here
	// as we need to update width/height later on.
	app = instantiateApp(graphics);

	// Window size policy is like macOS (see MacAppDelegate's setupWindow): start
	// from the app-requested size set in run.cpp's instantiateApp() rather than
	// deriving it from the monitor. If it doesn't fit, scale it down uniformly so
	// the original aspect ratio is preserved (leaving room for decorations).
	GLFWmonitor *primaryMonitor = glfwGetPrimaryMonitor();
	if (primaryMonitor != nullptr) {
		const GLFWvidmode *currentVideoMode = glfwGetVideoMode(primaryMonitor);
		if (currentVideoMode != nullptr) {
			float maxHeight = currentVideoMode->height * 0.9f; // room for decorations
			float maxWidth	= currentVideoMode->width;
			float scale		= 1.0f;
			if (graphics.height > maxHeight) {
				float s = maxHeight / graphics.height;
				if (s < scale) scale = s;
			}
			if (graphics.width > maxWidth) {
				float s = maxWidth / graphics.width;
				if (s < scale) scale = s;
			}
			if (scale < 1.0f) {
				graphics.width	= (int) (graphics.width * scale);
				graphics.height = (int) (graphics.height * scale);
			}
		}
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
		// MSAA to match the OpenGL path's multisampling (the GL backend uses
		// GLFW_SAMPLES). D3D11 guarantees 4x support; the swapchain stays 1x and
		// sokol resolves the MSAA render target into it at end-of-pass.
		if (!d3d11.init(hwnd, graphics.width, graphics.height, mzglSokolSampleCount)) {
			Log::e() << "Failed to initialize D3D11";
			glfwDestroyWindow(window);
			glfwTerminate();
			exit(EXIT_FAILURE);
		}

		sg_environment env		  = {};
		env.defaults.sample_count = d3d11.getSampleCount();
		env.defaults.color_format = SG_PIXELFORMAT_BGRA8;
		env.defaults.depth_format = SG_PIXELFORMAT_NONE;
		env.d3d11.device		  = d3d11.getDevice();
		env.d3d11.device_context  = d3d11.getDeviceContext();
		mzglSokolSetup(env);

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

#elif defined(MZGL_SOKOL) && defined(__EMSCRIPTEN__)
	// GLES3 (WebGL2) + Sokol backend for the browser.
	glfwMakeContextCurrent(window);

	// GLFW reports the requested window size, but the emscripten GLFW port does
	// not keep the actual <canvas> drawing buffer matched to it. Set it here so
	// the GL viewport, ortho projection and canvas all agree on the first frame
	// (the main loop re-asserts it thereafter).
	emscripten_set_canvas_element_size("#canvas", graphics.width, graphics.height);

	{
		sg_environment env		  = {};
		env.defaults.sample_count = mzglSokolSampleCount; // matches GLFW_SAMPLES
		mzglSokolSetup(env);
	}

	initMZGL(app);
	eventDispatcher->setup();

	gEmscriptenRunner = this;
	// fps=0 -> drive from requestAnimationFrame. simulate_infinite_loop=false so
	// run()/main() return normally; emscripten keeps the runtime alive because a
	// main loop is registered (EXIT_RUNTIME defaults to 0).
	emscripten_set_main_loop(emscriptenMainLoop, 0, false);

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
#	ifdef _WIN32
			// VST3 plugin editors that host their own OpenGL view (Cardinal,
			// JUCE-OpenGL plugins, etc.) call wglMakeCurrent inside their
			// WM_PAINT to bind their own context, and never restore ours. Once
			// that happens every subsequent Koala GL call goes to the wrong
			// context and Koala renders black. Re-binding our context every
			// frame is cheap and immune to whatever the plugin does.
			glfwMakeContextCurrent(window);
#	endif
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

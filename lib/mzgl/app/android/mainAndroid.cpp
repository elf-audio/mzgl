#include <EGL/egl.h>
#include <GLES/gl.h>
#include "util.h"
#include "EventDispatcher.h"
#include <android_native_app_glue.h>
#include <util/log.h>
#include "Graphics.h"
#include "androidUtil.h"
#include "Shader.h"
#include "Texture.h"

#include "androidKeyCodes.h"

Graphics graphics;

std::shared_ptr<EventDispatcher> eventDispatcher = nullptr;
std::shared_ptr<App> app						 = nullptr;

class RenderEngine {
public:
	struct android_app *app;

	EGLDisplay display		  = nullptr;
	EGLSurface surface		  = nullptr;
	EGLContext context		  = nullptr;
	EGLConfig config		  = nullptr;
	bool clearedUpGLResources = false;

	int32_t width  = 0;
	int32_t height = 0;

	void chooseConfig() {
		EGLint numConfigs;

		/*
     * Here specify the attributes of the desired configuration.
     * Below, we select an EGLConfig with at least 8 bits per color
     * component compatible with on-screen windows
     */
		const EGLint attribs[] = {EGL_SURFACE_TYPE,
								  EGL_WINDOW_BIT,
								  EGL_BLUE_SIZE,
								  8,
								  EGL_GREEN_SIZE,
								  8,
								  EGL_RED_SIZE,
								  8,
								  EGL_RENDERABLE_TYPE,
								  EGL_OPENGL_ES2_BIT,
								  //            EGL_NATIVE_RENDERABLE, EGL_TRUE,
								  //            EGL_RENDERABLE_TYPE,
								  // EGL_ALPHA_SIZE, 8,
								  EGL_NONE};

		/* Here, the application chooses the configuration it desires.
         * find the best match if possible, otherwise use the very first one
         */
		eglChooseConfig(display, attribs, nullptr, 0, &numConfigs);
		std::unique_ptr<EGLConfig[]> supportedConfigs(new EGLConfig[numConfigs]);
		eglChooseConfig(display, attribs, supportedConfigs.get(), numConfigs, &numConfigs);

		auto i = 0;

		for (; i < numConfigs; i++) {
			auto &cfg = supportedConfigs[i];
			EGLint r, g, b, d;
			if (eglGetConfigAttrib(display, cfg, EGL_RED_SIZE, &r)
				&& eglGetConfigAttrib(display, cfg, EGL_GREEN_SIZE, &g)
				&& eglGetConfigAttrib(display, cfg, EGL_BLUE_SIZE, &b)
				&& eglGetConfigAttrib(display, cfg, EGL_DEPTH_SIZE, &d) && r == 8 && g == 8 && b == 8 && d == 0) {
				config = supportedConfigs[i];
				break;
			}
		}

		Log::i() << "Found " << numConfigs << " configs, using config " << i;
		if (i == numConfigs) {
			Log::w() << "Didn't find a supported config, so going with first (default)";
			config = supportedConfigs[0];
		}
	}

	/**
	 * Initialize an EGL context for the current display.
 	 */
	int initDisplay() {
		display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

		EGLint major;
		EGLint minor;
		eglInitialize(display, &major, &minor);
		Log::e() << "EGL VERSION " << major << "." << minor;

		chooseConfig();

		createSurface();

		////////////////////////////////////////////////////////////////////////////////////////////////
		/// now make the context

		EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};

		context = eglCreateContext(display, config, nullptr, contextAttribs);

		EGLint contextVersion;
		if (eglQueryContext(display, context, EGL_CONTEXT_CLIENT_VERSION, &contextVersion)) {
			Log::i() << "context version " << contextVersion;
		} else {
			Log::e() << "Couldn't query context version";
		}

		Log::i() << "About to set eglMakeCurrent";
		if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
			Log::w() << "Unable to eglMakeCurrent";
			return -1;
		}

		const GLubyte *glVersion = glGetString(GL_VERSION);
		if (glVersion != nullptr) {
			Log::d() << "GL VERSION: " << glVersion;
		} else {
			Log::d() << "GL VERSION WAS NULL";
		}

		return 0;
	}

	void createSurface() {
		EGLint format;
		/* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
         * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
         * As soon as we picked a EGLConfig, we can safely reconfigure the
         * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
		eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
		surface = eglCreateWindowSurface(display, config, app->window, nullptr);

		eglQuerySurface(display, surface, EGL_WIDTH, &width);
		eglQuerySurface(display, surface, EGL_HEIGHT, &height);
	}

	bool prepareFrame() {
		if (display == nullptr) {
			return false;
		}

		auto err = glGetError();
		if (err != GL_NO_ERROR) {
			LOGE("GL error in engine_draw_frame(): 0x%08x\n", err);
			if (surface == nullptr) {
				createSurface();
			}
		}

		EGLint outInt;
		auto result = eglQueryContext(display, context, EGL_CONFIG_ID, &outInt);
		if (result != EGL_TRUE) {
			return false;
		}
		return true;
	}

	/**
     * Tear down the EGL context currently associated with the display.
     */
	void termDisplay() {
		graphics.clearUpResources();
		clearedUpGLResources = true;
		if (display != nullptr) {
			eglMakeCurrent(display, nullptr, nullptr, nullptr);
			if (context != nullptr) {
				eglDestroyContext(display, context);
			}
			if (surface != nullptr) {
				eglDestroySurface(display, surface);
			}
			eglTerminate(display);
		}

		display = nullptr;
		context = nullptr;
		surface = nullptr;
	}
	[[nodiscard]] bool ready() const { return display != nullptr; }
};
std::shared_ptr<RenderEngine> engine = nullptr;

bool firstFrameAlreadyRendered = false;

std::shared_ptr<App> androidGetApp() {
	return app;
}

/**
 * Just the current frame in the display.
 */
static void engine_draw_frame() {
	if (!engine->prepareFrame()) {
		return;
	}

	if (engine->display == nullptr || engine->surface == nullptr || eglGetCurrentContext() == nullptr) {
		return;
	}

	if (!firstFrameAlreadyRendered) {
		graphics.width	= engine->width;
		graphics.height = engine->height;
		glViewport(0, 0, graphics.width, graphics.height);

		// just draw *something* whilst loading... - doesn't seem to work
		eventDispatcher->androidDrawLoading();
		if (!eglSwapBuffers(engine->display, engine->surface)) {
			firstFrameAlreadyRendered = false;
			return;
		}

		eventDispatcher->setup();
		firstFrameAlreadyRendered = true;
	}

	{
		// ugh, super ugly, but this checks for orientation changes
		int wBefore = engine->width;
		if (!eglQuerySurface(engine->display, engine->surface, EGL_WIDTH, &engine->width)
			|| !eglQuerySurface(engine->display, engine->surface, EGL_HEIGHT, &engine->height)) {
			return;
		}

		if (wBefore != engine->width) {
			graphics.width	= engine->width;
			graphics.height = engine->height;
			glViewport(0, 0, graphics.width, graphics.height);
			eventDispatcher->resized();
		}
	}

	eventDispatcher->runFrame();

	if (!eglSwapBuffers(engine->display, engine->surface)) {
		EGLint error = eglGetError();
		if (error == EGL_BAD_SURFACE || error == EGL_CONTEXT_LOST) {
			firstFrameAlreadyRendered = false;
		}
	}
}

/**
 * Process the next input event.
 * Return 1 if you handle an event, 0 if you don't.
 */
static int32_t engine_handle_input(struct android_app *androidApp, AInputEvent *event) {
	// converted from Java in openframeworks android
	if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
		int32_t action = AMotionEvent_getAction(event);
		int32_t pointerIndex =
			(action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

		// TODO: android touch Id doesn't work here
		//uint32_t touchId = AMotionEvent_getPointerId(event, pointerIndex);

		switch (action & AMOTION_EVENT_ACTION_MASK) {
			case AMOTION_EVENT_ACTION_MOVE:
				for (int i = 0; i < AMotionEvent_getHistorySize(event); i++) {
					for (int j = 0; j < AMotionEvent_getPointerCount(event); j++) {
						//LOGE("touch move : %d %d %d\n", x, y, touchId);
						eventDispatcher->touchMoved(AMotionEvent_getHistoricalX(event, j, i),
													AMotionEvent_getHistoricalY(event, j, i),
													AMotionEvent_getPointerId(event, j));
					}
				}
				for (int i = 0; i < AMotionEvent_getPointerCount(event); i++) {
					eventDispatcher->touchMoved(AMotionEvent_getX(event, i),
												AMotionEvent_getY(event, i),
												AMotionEvent_getPointerId(event, i));
				}
				break;

			case AMOTION_EVENT_ACTION_DOWN:
			case AMOTION_EVENT_ACTION_POINTER_DOWN:
				eventDispatcher->touchDown(AMotionEvent_getX(event, pointerIndex),
										   AMotionEvent_getY(event, pointerIndex),
										   AMotionEvent_getPointerId(event, pointerIndex));

				break;
			case AMOTION_EVENT_ACTION_UP:
			case AMOTION_EVENT_ACTION_POINTER_UP:
				eventDispatcher->touchUp(AMotionEvent_getX(event, pointerIndex),
										 AMotionEvent_getY(event, pointerIndex),
										 AMotionEvent_getPointerId(event, pointerIndex));
				break;

			default:
				return 0;
				//LOGE("Unhandled motion type %d", flags);
				break;
				/*
                AMOTION_EVENT_ACTION_OUTSIDE = 4,
                AMOTION_EVENT_ACTION_POINTER_DOWN = 5,
                AMOTION_EVENT_ACTION_POINTER_UP = 6,
                AMOTION_EVENT_ACTION_HOVER_MOVE = 7,
                AMOTION_EVENT_ACTION_SCROLL = 8,
                AMOTION_EVENT_ACTION_HOVER_ENTER = 9,
                AMOTION_EVENT_ACTION_HOVER_EXIT = 10,
                AMOTION_EVENT_ACTION_BUTTON_PRESS = 11,
                AMOTION_EVENT_ACTION_BUTTON_RELEASE = 12*/
		}

		//LOGE("touch : %d %d\n", x, y);

		return 1; // event handled
	} else if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY) {
		int32_t keyAction = AKeyEvent_getAction(event);
		bool shiftIsHeld  = AKeyEvent_getMetaState(event) & AMETA_SHIFT_ON;
		if (keyAction == AKEY_EVENT_ACTION_DOWN) {
			auto key = keycodeToKey(AKeyEvent_getKeyCode(event), shiftIsHeld);
			if (key != 0) {
				eventDispatcher->keyDown(key);
				return 1;
			}
			// if we don't  recognize the key, return 0 for 'not handled'
			// it may be the volume controls and we want the OS to deal with that.
			return 0;
		} else if (keyAction == AKEY_EVENT_ACTION_UP) {
			auto key = keycodeToKey(AKeyEvent_getKeyCode(event), shiftIsHeld);
			if (key != 0) {
				eventDispatcher->keyUp(key);
				return 1;
			}
			// if we don't recognize the key, return 0 for 'not handled'
			// it may be the volume controls and we want the OS to deal with that.
			return 0;
		} else if (keyAction == AKEY_EVENT_ACTION_MULTIPLE) {
			Log::e() << "Unhandled AKEY_EVENT_ACTION_MULTIPLE";
			return 0;
		}
		return 1;
	}
	return 0; // event not handled
}

/**
 * Process the next main command.
 */
static void engine_handle_cmd(struct android_app *appPtr, int32_t cmd) {
	if (!eventDispatcher || !eventDispatcher->app) return;
	switch (cmd) {
		case APP_CMD_INIT_WINDOW:
			Log::d() << "APP_CMD_INIT_WINDOW";
			// The window is being shown, get it ready.
			if (engine->app->window != nullptr) {
				engine->initDisplay();

				initMZGL(app);
				if (engine->prepareFrame()) {
					graphics.width	= engine->width;
					graphics.height = engine->height;
					eventDispatcher->androidDrawLoading();
					eglSwapBuffers(engine->display, engine->surface);
				}
				if (engine->clearedUpGLResources) {
					eventDispatcher->resized();
					eventDispatcher->willEnterForeground(); // THIS IS IMPORTANT BUT IT MAKES IT CRASH!!!
					engine->clearedUpGLResources = false;
				}
				engine_draw_frame();
			}
			break;

		case APP_CMD_TERM_WINDOW:
			// The window is being hidden or closed, clean it up.
			Log::d() << "APP_CMD_TERM_WINDOW";
			engine->termDisplay();
			break;

		case APP_CMD_LOST_FOCUS: Log::d() << "APP_CMD_LOST_FOCUS"; break;

		case APP_CMD_CONFIG_CHANGED: Log::e() << "APP_CMD_CONFIG_CHANGED"; break;

		case APP_CMD_START: Log::d() << "APP_CMD_START"; break;

		case APP_CMD_STOP:
			Log::d() << "APP_CMD_STOP";
			eventDispatcher->didEnterBackground();
			break;

		case APP_CMD_SAVE_STATE: Log::d() << "APP_CMD_SAVE_STATE"; break;

		case APP_CMD_WINDOW_RESIZED: Log::e() << "APP_CMD_WINDOW_RESIZED"; break;

		case APP_CMD_RESUME:
			Log::d() << "APP_CMD_RESUME";
			eventDispatcher->androidOnResume();
			break;

		case APP_CMD_DESTROY:
			LOGE("APP_CMD_DESTROY");
			eventDispatcher->exit();
			break;

		case APP_CMD_LOW_MEMORY:
			Log::d() << "APP_CMD_LOW_MEMORY";
			eventDispatcher->memoryWarning();
			break;

		case APP_CMD_PAUSE:
			Log::d() << "APP_CMD_PAUSE";
			eventDispatcher->androidOnPause();
			break;
	}
}

android_app *getAndroidAppPtr() {
	return engine->app;
}

EventDispatcher *getAndroidEventDispatcher() {
	return eventDispatcher.get();
}

int android_loop_all(int timeoutMillis, int *outFd, int *outEvents, void **outData) {
	int result;
	do {
		result = ALooper_pollOnce(timeoutMillis, outFd, outEvents, outData);
	} while (result == ALOOPER_POLL_CALLBACK);
	return result;
}

static void runLoop(android_app *state) {
	// loop waiting for stuff to do.
	while (true) {
		// Read all pending events.
		int events;
		struct android_poll_source *source;

		while (android_loop_all(engine->ready() ? 0 : -1, nullptr, &events, (void **) &source) >= 0) {
			// Process this event.
			if (source != nullptr) {
				source->process(state, source);
			}

			// Check if we are exiting.
			if (state->destroyRequested != 0) {
				engine->termDisplay();
				return;
			}
		}

		engine_draw_frame();
	}
}
/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(android_app *state) {
	state->onAppCmd		= engine_handle_cmd;
	state->onInputEvent = engine_handle_input;
	engine				= std::make_shared<RenderEngine>(state);

	app				= instantiateApp(graphics);
	eventDispatcher = make_shared<EventDispatcher>(app);

	runLoop(state);
	engine			= nullptr;
	eventDispatcher = nullptr;
	app				= nullptr;
}

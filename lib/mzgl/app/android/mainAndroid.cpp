#include <EGL/egl.h>
#include <GLES/gl.h>
#include "util.h"
#include "EventDispatcher.h"
#include <android_native_app_glue.h>
#include <util/log.h>

#include "androidUtil.h"
#include "Shader.h"
#include "Texture.h"

using namespace std;

bool engineReady = false;

/**
 * Shared state for our app.
 */
struct AppEngine {
	android_app *app = nullptr;

	EGLDisplay display = nullptr;
	EGLSurface surface = nullptr;
	EGLContext context = nullptr;
	EGLConfig config;
	int32_t width  = 0;
	int32_t height = 0;
};

#include "Graphics.h"

bool firstFrameAlreadyRendered = false;
Graphics graphics;

shared_ptr<EventDispatcher> eventDispatcher = nullptr;
shared_ptr<App> app							= nullptr;

shared_ptr<App> androidGetApp() {
	return app;
}

void chooseConfig(AppEngine *engine) {
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
	eglChooseConfig(engine->display, attribs, nullptr, 0, &numConfigs);
	std::unique_ptr<EGLConfig[]> supportedConfigs(new EGLConfig[numConfigs]);
	assert(supportedConfigs);
	eglChooseConfig(engine->display, attribs, supportedConfigs.get(), numConfigs, &numConfigs);
	assert(numConfigs);

	auto i = 0;

	for (; i < numConfigs; i++) {
		auto &cfg = supportedConfigs[i];
		EGLint r, g, b, /*a, */ d;
		if (eglGetConfigAttrib(engine->display, cfg, EGL_RED_SIZE, &r)
			&& eglGetConfigAttrib(engine->display, cfg, EGL_GREEN_SIZE, &g)
			&& eglGetConfigAttrib(engine->display, cfg, EGL_BLUE_SIZE, &b) &&
			//eglGetConfigAttrib(display, cfg, EGL_ALPHA_SIZE, &a)  &&
			eglGetConfigAttrib(engine->display, cfg, EGL_DEPTH_SIZE, &d) && r == 8 && g == 8
			&& b == 8 /*&& a == 8*/ && d == 0) {
			engine->config = supportedConfigs[i];
			break;
		}
	}

	LOGI("Found %d configs, using config %d", numConfigs, i);
	if (i == numConfigs) {
		LOGI("Didn't find a supported config, so going with first (default)");
		engine->config = supportedConfigs[0];
	}
}

// must have created a config and display before this
void createSurface(AppEngine *engine) {
	EGLint format;
	/* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
     * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
     * As soon as we picked a EGLConfig, we can safely reconfigure the
     * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
	eglGetConfigAttrib(engine->display, engine->config, EGL_NATIVE_VISUAL_ID, &format);
	engine->surface = eglCreateWindowSurface(engine->display, engine->config, engine->app->window, nullptr);

	eglQuerySurface(engine->display, engine->surface, EGL_WIDTH, &engine->width);
	eglQuerySurface(engine->display, engine->surface, EGL_HEIGHT, &engine->height);
}

/**
 * Initialize an EGL context for the current display.
 */
static int engine_init_display(AppEngine *engine) {
	engine->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	EGLint major;
	EGLint minor;
	eglInitialize(engine->display, &major, &minor);
	LOGE("EGL VERSION %d.%d", major, minor);

	chooseConfig(engine);

	createSurface(engine);

	////////////////////////////////////////////////////////////////////////////////////////////////
	/// now make the context

	EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};

	engine->context = eglCreateContext(engine->display, engine->config, nullptr, contextAttribs);

	EGLint contextVersion;
	if (eglQueryContext(engine->display, engine->context, EGL_CONTEXT_CLIENT_VERSION, &contextVersion)) {
		LOGE("context version %d", contextVersion);
	} else {
		LOGE("Couldn't query context version");
	}

	LOGI("About to set eglMakeCurrent");
	if (eglMakeCurrent(engine->display, engine->surface, engine->surface, engine->context) == EGL_FALSE) {
		LOGW("Unable to eglMakeCurrent");
		return -1;
	}

	const GLubyte *glVersion = glGetString(GL_VERSION);
	if (glVersion != nullptr) {
		LOGE("GL VERSION: %s", glVersion);
	} else {
		LOGE("GL VERSION WAS NULL");
	}

	return 0;
}

static void engine_term_display(AppEngine *engine);

static bool prepareFrame(AppEngine *engine) {
	if (engine->display == nullptr) {
		// No display.
		return false;
	}

	auto err = glGetError();
	if (err != GL_NO_ERROR) {
		LOGE("GL error in engine_draw_frame(): 0x%08x\n", err);
		if (engine->surface == nullptr) {
			createSurface(engine);
		}
	}

	EGLint outInt;
	auto result = eglQueryContext(engine->display, engine->context, EGL_CONFIG_ID, &outInt);
	if (result != EGL_TRUE) {
		Log::e() << "Got false";
		return false;
	}
	return true;
}

/**
 * Just the current frame in the display.
 */
static void engine_draw_frame(AppEngine *engine) {
	if (!prepareFrame(engine)) {
		return;
	}

	if (!firstFrameAlreadyRendered) {
		graphics.width	= engine->width;
		graphics.height = engine->height;
		glViewport(0, 0, graphics.width, graphics.height);

		eventDispatcher = make_shared<EventDispatcher>(app);
		// just draw *something* whilst loading... - doesn't seem to work
		eventDispatcher->androidDrawLoading();
		eglSwapBuffers(engine->display, engine->surface);

		eventDispatcher->setup();
		firstFrameAlreadyRendered = true;
	}

	{
		// ugh, super ugly, but this checks for orientation changes
		int wBefore = engine->width;
		eglQuerySurface(engine->display, engine->surface, EGL_WIDTH, &engine->width);
		eglQuerySurface(engine->display, engine->surface, EGL_HEIGHT, &engine->height);

		if (wBefore != engine->width) {
			graphics.width	= engine->width;
			graphics.height = engine->height;
			glViewport(0, 0, graphics.width, graphics.height);
			eventDispatcher->resized();
		}
	}

	if (eventDispatcher != nullptr) {
		eventDispatcher->runFrame();
	} else {
		firstFrameAlreadyRendered = false;
	}

	eglSwapBuffers(engine->display, engine->surface);
}

/**
 * Tear down the EGL context currently associated with the display.
 */
static void engine_term_display(AppEngine *engine) {
	if (engine->display != EGL_NO_DISPLAY) {
		eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if (engine->context != EGL_NO_CONTEXT) {
			eglDestroyContext(engine->display, engine->context);
		}
		if (engine->surface != EGL_NO_SURFACE) {
			eglDestroySurface(engine->display, engine->surface);
		}
		eglTerminate(engine->display);
	}

	engine->display = EGL_NO_DISPLAY;
	engine->context = EGL_NO_CONTEXT;
	engine->surface = EGL_NO_SURFACE;
}

#include "androidKeyCodes.h"

/**
 * Process the next input event.
 * Return 1 if you handle an event, 0 if you don't.
 */
static int32_t engine_handle_input(android_app *app, AInputEvent *event) {
	//engine* engine = (engine*)app->userData;
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
				//LOGW("pointer down %d", touchId);
				eventDispatcher->touchDown(AMotionEvent_getX(event, pointerIndex),
										   AMotionEvent_getY(event, pointerIndex),
										   AMotionEvent_getPointerId(event, pointerIndex));

				break;
			case AMOTION_EVENT_ACTION_UP:
			case AMOTION_EVENT_ACTION_POINTER_UP:
				//LOGW("pointer up %d", touchId);
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

bool clearedUpGLResources = false;

/**
 * Process the next main command.
 */
static void engine_handle_cmd(android_app *appPtr, int32_t cmd) {
	AppEngine *engine = (AppEngine *) appPtr->userData;

	switch (cmd) {
		case APP_CMD_INIT_WINDOW:
			Log::d() << "APP_CMD_INIT_WINDOW";
			// The window is being shown, get it ready.
			if (engine->app->window != nullptr) {
				engine_init_display(engine);
				engineReady = true;

				initMZGL(app);
				if (eventDispatcher != nullptr && prepareFrame(engine)) {
					//                    engine_draw_blankFrame(engine);
					graphics.width	= engine->width;
					graphics.height = engine->height;
					eventDispatcher->androidDrawLoading();
					eglSwapBuffers(engine->display, engine->surface);
				}
				if (clearedUpGLResources && eventDispatcher != nullptr) {
					eventDispatcher->resized();
					eventDispatcher->willEnterForeground(); // THIS IS IMPORTANT BUT IT MAKES IT CRASH!!!
					clearedUpGLResources = false;
				}
				engine_draw_frame(engine);
			}
			break;

		case APP_CMD_TERM_WINDOW:
			// The window is being hidden or closed, clean it up.
			Log::d() << "APP_CMD_TERM_WINDOW";
			graphics.clearUpResources();
			clearedUpGLResources = true;
			engine_term_display(engine);
			engineReady = false;
			break;

		case APP_CMD_LOST_FOCUS: Log::d() << "APP_CMD_LOST_FOCUS"; break;

		case APP_CMD_CONFIG_CHANGED: Log::e() << "APP_CMD_CONFIG_CHANGED"; break;

		case APP_CMD_START: Log::d() << "APP_CMD_START"; break;

		case APP_CMD_STOP:
			Log::d() << "APP_CMD_STOP";
			if (eventDispatcher && eventDispatcher->app != nullptr) {
				eventDispatcher->didEnterBackground();
			}
			break;

		case APP_CMD_SAVE_STATE: Log::d() << "APP_CMD_SAVE_STATE"; break;

		case APP_CMD_WINDOW_RESIZED: Log::e() << "APP_CMD_WINDOW_RESIZED"; break;

		case APP_CMD_RESUME:
			Log::d() << "APP_CMD_RESUME";
			if (eventDispatcher && eventDispatcher->app != nullptr) {
				eventDispatcher->androidOnResume();
			}
			break;

		case APP_CMD_DESTROY:
			LOGE("APP_CMD_DESTROY");
			if (eventDispatcher != nullptr) {
				eventDispatcher->exit();
			}
			break;

		case APP_CMD_LOW_MEMORY:
			Log::d() << "APP_CMD_LOW_MEMORY";
			if (eventDispatcher != nullptr) {
				eventDispatcher->memoryWarning();
			}
			break;

		case APP_CMD_PAUSE:
			Log::d() << "APP_CMD_PAUSE";
			if (eventDispatcher && eventDispatcher->app != nullptr) {
				eventDispatcher->androidOnPause();
			}
			break;
	}
}

android_app *globalAppPtr = nullptr;

android_app *getAndroidAppPtr() {
	return globalAppPtr;
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

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(android_app *state) {
	app				= nullptr;
	eventDispatcher = nullptr;

	globalAppPtr = state;
	AppEngine engine;

	memset(&engine, 0, sizeof(AppEngine));
	state->userData		= &engine;
	state->onAppCmd		= engine_handle_cmd;
	state->onInputEvent = engine_handle_input;
	engine.app			= state;

	app = instantiateApp(graphics);

	// loop waiting for stuff to do.
	while (true) {
		// Read all pending events.
		int ident;
		int events;
		android_poll_source *source = nullptr;

		while ((ident = android_loop_all(engineReady ? 0 : -1, nullptr, &events, (void **) &source)) >= 0) {
			// Process this event.
			if (source != nullptr) {
				source->process(state, source);
			}

			// Check if we are exiting.
			if (state->destroyRequested != 0) {
				engine_term_display(&engine);
				return;
			}
		}

		engine_draw_frame(&engine);
	}
}

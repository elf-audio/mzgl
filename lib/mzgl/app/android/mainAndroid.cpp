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

EGLConfig chooseConfig(EGLDisplay display) {
	EGLConfig config = nullptr;

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
	return config;
}

class RenderEngine {
public:
	android_app *androidApp;

	explicit RenderEngine(android_app *_app)
		: androidApp(_app) {
		display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	}
	bool hasFocus			  = false;
	EGLDisplay display		  = nullptr;
	EGLSurface surface		  = nullptr;
	EGLContext context		  = nullptr;
	bool clearedUpGLResources = false;

	int32_t width  = 0;
	int32_t height = 0;

	void printContextVersion() const {
		EGLint contextVersion;
		if (eglQueryContext(display, context, EGL_CONTEXT_CLIENT_VERSION, &contextVersion)) {
			Log::i() << "context version " << contextVersion;
		} else {
			Log::e() << "Couldn't query context version";
		}
	}
	
	void initWindow() {
		display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

		EGLint major, minor;
		eglInitialize(display, &major, &minor);
		Log::d() << "EGL VERSION " << major << "." << minor;

		auto config = chooseConfig(display);

		surface = createSurface(config);

		EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
		context					= eglCreateContext(display, config, nullptr, contextAttribs);

		printContextVersion();

		if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
			Log::w() << "Unable to eglMakeCurrent";
		}
	}

	bool getSurfaceDims(EGLSurface surf, int &w, int &h) const {
		return (eglQuerySurface(display, surf, EGL_WIDTH, &w) && eglQuerySurface(display, surf, EGL_HEIGHT, &h));
	}

	EGLSurface createSurface(EGLConfig config) {
		EGLSurface surf;
		surf = eglCreateWindowSurface(display, config, androidApp->window, nullptr);
		getSurfaceDims(surf, width, height);
		graphics.width	= width;
		graphics.height = height;
		return surf;
	}

	void drawFrame() {
		if (surface == nullptr || eglGetCurrentContext() == nullptr) {
			return;
		}

		if (!firstFrameAlreadyRendered) {
			graphics.width	= width;
			graphics.height = height;

			glViewport(0, 0, graphics.width, graphics.height);
			eventDispatcher->androidDrawLoading();
			eglSwapBuffers(display, surface);

			eventDispatcher->setup();
			firstFrameAlreadyRendered = true;
		}

		{
			// ugh, super ugly, but this checks for orientation changes
			int wBefore = width;
			if (!getSurfaceDims(surface, width, height)) {
				return;
			}

			if (wBefore != width) {
				graphics.width	= width;
				graphics.height = height;
				glViewport(0, 0, graphics.width, graphics.height);
				eventDispatcher->resized();
			}
		}

		eventDispatcher->runFrame();

		eglSwapBuffers(display, surface);

		auto err = glGetError();
		if (err != GL_NO_ERROR) {
			Log::e() << "GL error in engine_draw_frame(): " << err;
		}
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

	static void handleCmdStatic(android_app *appPtr, int32_t cmd) {
		auto *_engine = static_cast<RenderEngine *>(appPtr->userData);
		_engine->handleCmd(cmd);
	}
	void handleCmd(int32_t cmd) {
		if (!eventDispatcher || !eventDispatcher->app) return;
		switch (cmd) {
			case APP_CMD_INIT_WINDOW:
				Log::d() << "APP_CMD_INIT_WINDOW";
				// The window is being shown, get it ready.
				if (androidApp->window == nullptr) {
					Log::e() << "androidApp->window is null in APP_CMD_INIT_WINDOW";
					return;
				}
				initWindow();
				initMZGL(app);

				eventDispatcher->androidDrawLoading();
				eglSwapBuffers(display, surface);

				if (clearedUpGLResources) {
					eventDispatcher->resized();
					eventDispatcher->willEnterForeground(); // THIS IS IMPORTANT BUT IT MAKES IT CRASH!!!
					clearedUpGLResources = false;
				}
				hasFocus = true;
				drawFrame();

				break;

			case APP_CMD_TERM_WINDOW:
				// The window is being hidden or closed, clean it up.
				Log::d() << "APP_CMD_TERM_WINDOW";
				termDisplay();
				hasFocus = false;
				break;

			case APP_CMD_GAINED_FOCUS:
				Log::d() << "APP_CMD_GAINED_FOCUS";
				hasFocus = true;
				break;
			case APP_CMD_LOST_FOCUS:
				Log::d() << "APP_CMD_LOST_FOCUS";
				hasFocus = false;
				break;

			case APP_CMD_CONFIG_CHANGED: Log::e() << "APP_CMD_CONFIG_CHANGED"; break;
			case APP_CMD_CONTENT_RECT_CHANGED: Log::d() << "APP_CMD_CONTENT_RECT_CHANGED"; break;
			case APP_CMD_WINDOW_REDRAW_NEEDED: Log::d() << "APP_CMD_WINDOW_REDRAW_NEEDED"; break;
			case APP_CMD_INPUT_CHANGED: Log::d() << "APP_CMD_INPUT_CHANGED"; break;
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
	[[nodiscard]] bool ready() const { return hasFocus; }
	bool firstFrameAlreadyRendered = false;
};

std::shared_ptr<App> androidGetApp() {
	return app;
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

EventDispatcher *getAndroidEventDispatcher() {
	return eventDispatcher.get();
}

std::shared_ptr<RenderEngine> engine = nullptr;

android_app *getAndroidAppPtr() {
	return engine->androidApp;
}

void android_main(android_app *state) {
	engine				= std::make_shared<RenderEngine>(state);
	state->userData		= engine.get();
	state->onAppCmd		= RenderEngine::handleCmdStatic;
	state->onInputEvent = engine_handle_input;
	app					= instantiateApp(graphics);
	eventDispatcher		= make_shared<EventDispatcher>(app);

	while (!state->destroyRequested) {
		android_poll_source *source = nullptr;

		auto result = ALooper_pollOnce(engine->ready() ? 0 : -1, nullptr, nullptr, (void **) &source);

		if (result == ALOOPER_POLL_ERROR) {
			Log::e() << "ALooper_pollOnce returned an error";
			mzAssert(result != ALOOPER_POLL_ERROR, "ALooper_pollOnce returned an error");
		}

		if (source != nullptr) source->process(state, source);

		if (engine->ready()) {
			engine->drawFrame();
		}
	}
	engine->termDisplay();
}

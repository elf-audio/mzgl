#include <EGL/egl.h>
#include <GLES/gl.h>
#include "util.h"
#include "EventDispatcher.h"
#include <android_native_app_glue.h>
#include <util/log.h>
#include "Graphics.h"
#include "androidUtil.h"
#include "TextInput.h"
#include "Shader.h"
#include "Texture.h"

#include "androidKeyCodes.h"

#include <mutex>

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

			// hold the first layout until the system has delivered window
			// insets (they arrive async on the java UI thread) - otherwise
			// setup() lays out against zero insets and draws under the
			// navigation bar. Times out in case a device never delivers them.
			if (!androidSafeInsetsKnown()) {
				if (insetsWaitStartTime == 0.f) insetsWaitStartTime = getSeconds();
				if (getSeconds() - insetsWaitStartTime < 0.5f) {
					return; // keep showing the loading frame, try again next frame
				}
				Log::w() << "Timed out waiting for window insets, laying out without them";
			}

			eventDispatcher->setup();
			// route Graphics::showKeyboard/hideKeyboard to the Android soft keyboard
			graphics.onShowKeyboard = [](TextInputReceiver *r) {
				androidShowKeyboard(r ? r->getText() : std::string());
			};
			graphics.onHideKeyboard = []() { androidHideKeyboard(); };
			firstFrameAlreadyRendered = true;
		}

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

		eventDispatcher->runFrame();

		eglSwapBuffers(display, surface);

		auto err = glGetError();
		if (err != GL_NO_ERROR) {
			Log::e() << "GL error in engine_draw_frame(): " << err;
		}
	}

	void terminateDisplay() {
		Log::d() << "RenderEngine::terminateDisplay()";
		if (!clearedUpGLResources) {
			graphics.clearUpResources();
			clearedUpGLResources = true;
		}
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

	// False until this activity generation owns the shared globals
	// (app/eventDispatcher/engine/graphics). While false, lifecycle commands
	// and input must not be forwarded into the app - the globals still belong
	// to the previous activity instance. The glue still acks lifecycle state
	// changes internally, so the Java main thread is never blocked by this.
	bool generationActive = false;

	void handleCmd(int32_t cmd) {
		if (!generationActive) return;
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

				// Only forward resized()/willEnterForeground() into the app once
				// setup() has actually run (setup is deferred to the first
				// drawFrame, waiting for window insets) - before that the UI tree
				// doesn't exist yet and resized() null-derefs in doLayout.
				if (clearedUpGLResources && firstFrameAlreadyRendered) {
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
				terminateDisplay();
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

			case APP_CMD_STOP:
				Log::d() << "APP_CMD_STOP";
				eventDispatcher->didEnterBackground();
				break;

			case APP_CMD_RESUME:
				Log::d() << "APP_CMD_RESUME";
				eventDispatcher->androidOnResume();
				break;

			case APP_CMD_DESTROY:
				LOGE("APP_CMD_DESTROY");
				eventDispatcher->exit();
				//				eventDispatcher = nullptr;
				//				app				= nullptr;
				//				graphics.destroyResources();
				break;

			case APP_CMD_LOW_MEMORY:
				Log::d() << "APP_CMD_LOW_MEMORY";
				eventDispatcher->memoryWarning();
				break;

			case APP_CMD_PAUSE:
				Log::d() << "APP_CMD_PAUSE";
				eventDispatcher->androidOnPause();
				break;
			case APP_CMD_CONFIG_CHANGED: Log::e() << "APP_CMD_CONFIG_CHANGED"; break;
			case APP_CMD_CONTENT_RECT_CHANGED: Log::d() << "APP_CMD_CONTENT_RECT_CHANGED"; break;
			case APP_CMD_WINDOW_REDRAW_NEEDED: Log::d() << "APP_CMD_WINDOW_REDRAW_NEEDED"; break;
			case APP_CMD_INPUT_CHANGED: Log::d() << "APP_CMD_INPUT_CHANGED"; break;
			case APP_CMD_START: Log::d() << "APP_CMD_START"; break;
			case APP_CMD_SAVE_STATE: Log::d() << "APP_CMD_SAVE_STATE"; break;
			case APP_CMD_WINDOW_RESIZED: Log::e() << "APP_CMD_WINDOW_RESIZED"; break;
		}
	}
	[[nodiscard]] bool ready() const { return hasFocus; }
	bool firstFrameAlreadyRendered = false;
	float insetsWaitStartTime	   = 0.f;
};

std::shared_ptr<App> androidGetApp() {
	return app;
}

/**
 * Process the next input event.
 * Return 1 if you handle an event, 0 if you don't.
 */
static int32_t engine_handle_input(struct android_app *androidApp, AInputEvent *event) {
	auto *_engine = static_cast<RenderEngine *>(androidApp->userData);
	if (_engine == nullptr || !_engine->generationActive) return 0;
	if (!eventDispatcher || !eventDispatcher->app) return 0;
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

// Android can create the new NativeActivity (and native_app_glue its
// android_main thread) before the previous activity instance is destroyed.
// The app/engine/eventDispatcher/graphics globals belong to exactly one
// activity generation at a time, so a new android_main must not touch them
// until the previous one has completely torn down.
//
// The wait must NOT block this thread's command processing: Android only
// destroys the old activity after the new one has resumed, and NativeActivity
// lifecycle callbacks block the Java main thread until this thread's looper
// reads the corresponding glue command. A blocking wait here therefore
// deadlocks (new can't resume -> old never destroyed -> gate never opens).
// Instead the new generation pumps its looper with generationActive == false
// (commands/input are acked by the glue but not forwarded into the app) and
// polls for ownership.
static std::mutex lifecycleMutex;
static bool instanceRunning = false;

static bool tryAcquireGeneration() {
	std::lock_guard<std::mutex> lock(lifecycleMutex);
	if (instanceRunning) return false;
	instanceRunning = true;
	return true;
}

static void releaseGeneration() {
	std::lock_guard<std::mutex> lock(lifecycleMutex);
	instanceRunning = false;
}

void android_main(android_app *state) {
	auto myEngine		= std::make_shared<RenderEngine>(state);
	state->userData		= myEngine.get();
	state->onAppCmd		= RenderEngine::handleCmdStatic;
	state->onInputEvent = engine_handle_input;

	bool loggedWait = false;
	while (!tryAcquireGeneration()) {
		if (!loggedWait) {
			Log::i() << "android_main: waiting for previous activity instance to finish tearing down";
			loggedWait = true;
		}
		android_poll_source *source = nullptr;
		ALooper_pollOnce(50, nullptr, nullptr, (void **) &source);
		if (source != nullptr) source->process(state, source);
		if (state->destroyRequested) {
			// this activity was destroyed before the previous generation
			// finished tearing down - we never owned the globals, so there
			// is nothing to clean up
			Log::i() << "android_main: destroyed while waiting for previous generation";
			return;
		}
	}

	Log::i() << "android_main: starting activity generation";
	mzAssert(app == nullptr && engine == nullptr && eventDispatcher == nullptr,
			 "previous activity generation did not clean up");

	engine			= myEngine;
	app				= instantiateApp(graphics);
	eventDispatcher = make_shared<EventDispatcher>(app);

	myEngine->generationActive = true;
	// if the window/focus commands arrived while we were waiting for the
	// previous generation, replay them now that the app exists
	if (state->window != nullptr) {
		myEngine->handleCmd(APP_CMD_INIT_WINDOW);
		myEngine->handleCmd(APP_CMD_GAINED_FOCUS);
	}

	while (!state->destroyRequested) {
		android_poll_source *source = nullptr;

		auto result = ALooper_pollOnce(myEngine->ready() ? 0 : -1, nullptr, nullptr, (void **) &source);

		if (result == ALOOPER_POLL_ERROR) {
			Log::e() << "ALooper_pollOnce returned an error";
			mzAssert(result != ALOOPER_POLL_ERROR, "ALooper_pollOnce returned an error");
			continue;
		}

		if (source != nullptr) source->process(state, source);

		if (myEngine->ready()) {
			myEngine->drawFrame();
		}
	}

	// Tear down on THIS thread, while this generation's ANativeActivity and
	// EGL context still exist. Previously the app was destroyed lazily by the
	// *next* activity's android_main ("reset globals"), which ran the App
	// destructor on the wrong thread while this thread could still be inside
	// the render loop - the source of a whole family of teardown crashes
	// (wrong-thread main queue polling, layer tree deleted mid-iteration,
	// JNI calls through a freed activity, GL teardown racing GL init).
	Log::i() << "android_main: tearing down activity generation";
	myEngine->generationActive = false;
	eventDispatcher			   = nullptr;
	app						   = nullptr; // App destructor runs here, on its own thread, GL context still current
	myEngine->terminateDisplay();
	engine = nullptr;
	Log::i() << "android_main: teardown complete";

	releaseGeneration();
}

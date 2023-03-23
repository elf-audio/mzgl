

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
struct engine {
    struct android_app* app;

    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    EGLConfig config;
    int32_t width = 0;
    int32_t height = 0;
};


#include "Graphics.h"

bool firstFrameAlreadyRendered = false;
Graphics graphics;

shared_ptr<EventDispatcher> eventDispatcher = nullptr;
shared_ptr<App> app = nullptr;
shared_ptr<App> androidGetApp() {
    return app;
}

//EGLint __WIDTH = 0;
//EGLint __HEIGHT = 0;


//
//void
//MessageCallback( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam )
//{
//    string t = GL_DEBUG_TYPE_ERROR?"** GL ERROR **": "";
//Log::e() << "GL CALLBACK: "<<t<<", severity = "<<to_string(severity)<<" message = " << message;
//
//}



void chooseConfig(struct engine* engine) {


    EGLint numConfigs;

    /*
     * Here specify the attributes of the desired configuration.
     * Below, we select an EGLConfig with at least 8 bits per color
     * component compatible with on-screen windows
     */
    const EGLint attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
//            EGL_NATIVE_RENDERABLE, EGL_TRUE,
//            EGL_RENDERABLE_TYPE,
            // EGL_ALPHA_SIZE, 8,
            EGL_NONE
    };


    /* Here, the application chooses the configuration it desires.
     * find the best match if possible, otherwise use the very first one
     */
    eglChooseConfig(engine->display, attribs, nullptr,0, &numConfigs);
    std::unique_ptr<EGLConfig[]> supportedConfigs(new EGLConfig[numConfigs]);
    assert(supportedConfigs);
    eglChooseConfig(engine->display, attribs, supportedConfigs.get(), numConfigs, &numConfigs);
    assert(numConfigs);

    auto i = 0;

    for (; i < numConfigs; i++) {
        auto& cfg = supportedConfigs[i];
        EGLint r, g, b, /*a, */d;
        if (eglGetConfigAttrib(engine->display, cfg, EGL_RED_SIZE, &r)   &&
            eglGetConfigAttrib(engine->display, cfg, EGL_GREEN_SIZE, &g) &&
            eglGetConfigAttrib(engine->display, cfg, EGL_BLUE_SIZE, &b)  &&
            //eglGetConfigAttrib(display, cfg, EGL_ALPHA_SIZE, &a)  &&
            eglGetConfigAttrib(engine->display, cfg, EGL_DEPTH_SIZE, &d) &&
            r == 8 && g == 8 && b == 8 /*&& a == 8*/ && d == 0 ) {

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
void createSurface(struct engine *engine) {
    EGLint format;
    /* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
     * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
     * As soon as we picked a EGLConfig, we can safely reconfigure the
     * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
    eglGetConfigAttrib(engine->display, engine->config, EGL_NATIVE_VISUAL_ID, &format);
    engine->surface = eglCreateWindowSurface(engine->display, engine->config, engine->app->window, NULL);

    eglQuerySurface(engine->display, engine->surface, EGL_WIDTH, &engine->width);
    eglQuerySurface(engine->display, engine->surface, EGL_HEIGHT, &engine->height);
}

/**
 * Initialize an EGL context for the current display.
 */
static int engine_init_display(struct engine* engine) {
    // initialize OpenGL ES and EGL
//    LOGE("engine_init_display");



    engine->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    EGLint major;
    EGLint minor;
    eglInitialize(engine->display, &major, &minor);
    LOGE("EGL VERSION %d.%d", major, minor);


    chooseConfig(engine);


    createSurface(engine);



////////////////////////////////////////////////////////////////////////////////////////////////
/// now make the context

//    LOGI("About to set GLES version");
    EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };


    engine->context = eglCreateContext(engine->display, engine->config, NULL, contextAttribs);

    EGLint contextVersion;
    if(eglQueryContext(engine->display, engine->context, EGL_CONTEXT_CLIENT_VERSION, &contextVersion)) {
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
    if(glVersion!=nullptr) {
        LOGE("GL VERSION: %s", glVersion);
    } else {
        LOGE("GL VERSION WAS NULL");
    }




//    LOGI("Getting GL info");
//    // Check openGL on the system
//    auto opengl_info = {GL_VENDOR, GL_RENDERER, GL_VERSION, GL_EXTENSIONS};
//    for (auto name : opengl_info) {
//        auto info = glGetString(name);
//        LOGI("OpenGL Info: %s", info);
//    }

// During init, enable debug output
//    glEnable              ( GL_DEBUG_OUTPUT );
//    glDebugMessageCallback( MessageCallback, 0 );

    return 0;
}






bool checkGlError(const char* funcName) {
    GLint err = glGetError();
    if (err != GL_NO_ERROR) {
        LOGE("GL error after %s(): 0x%08x\n", funcName, err);
        return true;
    }
    return false;
}


static void engine_term_display(struct engine* engine);


static bool prepareFrame(struct engine* engine) {
    if (engine->display == NULL) {
        // No display.
        return false;
    }

    GLint err = glGetError();
    if (err != GL_NO_ERROR) {
        LOGE("GL error in engine_draw_frame(): 0x%08x\n", err);
        if(engine->surface==NULL) {
            createSurface(engine);
        }
    }

    EGLint outInt;
    auto result = eglQueryContext(engine->display, engine->context, EGL_CONFIG_ID, &outInt);
    if(result==EGL_FALSE) {
        Log::e() << "Got false";
        return false;
    } else if(result==EGL_TRUE) {
//        Log::d() << "Got true!";
    } else {
        Log::e() << "Got " << result;
        return false;
    }
    return true;
}
//
//static void engine_draw_blankFrame(struct engine* engine) {
//    if(!prepareFrame(engine)) {
//        return;
//    }
//
//    // just draw *something* whilst loading... - doesn't seem to work
//    graphics.clear(1.0, 0.1, 0.1, 1);
//    eglSwapBuffers(engine->display, engine->surface);
//}
/**
 * Just the current frame in the display.
 */
static void engine_draw_frame(struct engine* engine) {
    if(!prepareFrame(engine)) {
        return;
    }

    if(!firstFrameAlreadyRendered) {
        firstFrameAlreadyRendered = true;

        graphics.width = engine->width;
        graphics.height = engine->height;
        glViewport(0, 0, graphics.width, graphics.height);

        eventDispatcher = make_shared<EventDispatcher>(app);
        // just draw *something* whilst loading... - doesn't seem to work
        eventDispatcher->androidDrawLoading();
        eglSwapBuffers(engine->display, engine->surface);


        eventDispatcher->setup();
    }


    {
        // ugh, super ugly, but this checks for orientation changes
        int wBefore = engine->width;
        eglQuerySurface(engine->display, engine->surface, EGL_WIDTH, &engine->width);
        eglQuerySurface(engine->display, engine->surface, EGL_HEIGHT, &engine->height);

        if (wBefore != engine->width) {
            graphics.width = engine->width;
            graphics.height = engine->height;
            glViewport(0, 0, graphics.width, graphics.height);
            eventDispatcher->resized();
        }
    }

    if(eventDispatcher!=nullptr) {
        eventDispatcher->runFrame();
    } else {
        firstFrameAlreadyRendered = false;
    }

    eglSwapBuffers(engine->display, engine->surface);
}

/**
 * Tear down the EGL context currently associated with the display.
 */
static void engine_term_display(struct engine* engine) {
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

// get rid of this include, it's for fixing some touch problem in android koala
//#include "Global.h"


int keycodeToKey(int32_t k) {
    switch(k) {

        case AKEYCODE_0: return '0';
        case AKEYCODE_1: return '1';
        case AKEYCODE_2: return '2';
        case AKEYCODE_3: return '3';
        case AKEYCODE_4: return '4';
        case AKEYCODE_5: return '5';
        case AKEYCODE_6: return '6';
        case AKEYCODE_7: return '7';
        case AKEYCODE_8: return '8';
        case AKEYCODE_9: return '9';
        case AKEYCODE_STAR: return '*';

        case AKEYCODE_POUND: return '#';
        case AKEYCODE_A: return 'a';
        case AKEYCODE_B: return 'b';
        case AKEYCODE_C: return 'c';
        case AKEYCODE_D: return 'd';
        case AKEYCODE_E: return 'e';
        case AKEYCODE_F: return 'f';
        case AKEYCODE_G: return 'g';
        case AKEYCODE_H: return 'h';
        case AKEYCODE_I: return 'i';
        case AKEYCODE_J: return 'j';
        case AKEYCODE_K: return 'k';
        case AKEYCODE_L: return 'l';
        case AKEYCODE_M: return 'm';
        case AKEYCODE_N: return 'n';
        case AKEYCODE_O: return 'o';
        case AKEYCODE_P: return 'p';
        case AKEYCODE_Q: return 'q';
        case AKEYCODE_R: return 'r';
        case AKEYCODE_S: return 's';
        case AKEYCODE_T: return 't';
        case AKEYCODE_U: return 'u';
        case AKEYCODE_V: return 'v';
        case AKEYCODE_W: return 'w';
        case AKEYCODE_X: return 'x';
        case AKEYCODE_Y: return 'y';
        case AKEYCODE_Z: return 'z';
        case AKEYCODE_COMMA: return ',';
        case AKEYCODE_PERIOD: return '.';
        case AKEYCODE_TAB: return '\t';
        case AKEYCODE_SPACE: return ' ';
        case AKEYCODE_ENTER: return '\n';
        case AKEYCODE_DEL:  return 8;
        case AKEYCODE_GRAVE: return '`';
        case AKEYCODE_MINUS: return '-';
        case AKEYCODE_EQUALS: return '=';
        case AKEYCODE_LEFT_BRACKET: return '[';
        case AKEYCODE_RIGHT_BRACKET: return ']';
        case AKEYCODE_BACKSLASH: return '\\';

        case AKEYCODE_SEMICOLON: return ';';
        case AKEYCODE_APOSTROPHE: return '\'';
        case AKEYCODE_SLASH : return '/';
        case AKEYCODE_AT: return  '@';
        case AKEYCODE_PLUS : return  '+';
        case AKEYCODE_NUMPAD_0 : return '0';
        case AKEYCODE_NUMPAD_1 : return '1';
        case AKEYCODE_NUMPAD_2: return '2';
        case AKEYCODE_NUMPAD_3 : return  '3';
        case AKEYCODE_NUMPAD_4: return  '4';
        case AKEYCODE_NUMPAD_5: return  '5';
        case AKEYCODE_NUMPAD_6: return  '6';
        case AKEYCODE_NUMPAD_7 : return  '7';
        case AKEYCODE_NUMPAD_8: return  '8';
        case AKEYCODE_NUMPAD_9 : return '9';
        case AKEYCODE_NUMPAD_DIVIDE : return '/';
        case AKEYCODE_NUMPAD_MULTIPLY: return '*';
        case AKEYCODE_NUMPAD_SUBTRACT: return '-';
        case AKEYCODE_NUMPAD_ADD: return '+';
        case AKEYCODE_NUMPAD_DOT: return '.';
        case AKEYCODE_NUMPAD_COMMA: return ',';
        case AKEYCODE_NUMPAD_ENTER: return '\n';
        case AKEYCODE_NUMPAD_EQUALS: return '=';
        case AKEYCODE_NUMPAD_LEFT_PAREN : return '(';
        case AKEYCODE_NUMPAD_RIGHT_PAREN: return ')';
        default: return 0;
    }
}
/**
 * Process the next input event.
 * Return 1 if you handle an event, 0 if you don't.
 */
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
    //struct engine* engine = (struct engine*)app->userData;
    // converted from Java in openframeworks android
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {

        int32_t action = AMotionEvent_getAction(event);
        int32_t pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

        // TODO: android touch Id doesn't work here
        //uint32_t touchId = AMotionEvent_getPointerId(event, pointerIndex);

        switch(action & AMOTION_EVENT_ACTION_MASK) {
            case AMOTION_EVENT_ACTION_MOVE:
                for(int i = 0; i < AMotionEvent_getHistorySize(event); i++) {
                    for(int j = 0; j < AMotionEvent_getPointerCount(event); j++) {
                        //LOGE("touch move : %d %d %d\n", x, y, touchId);
                        eventDispatcher->touchMoved(
                                AMotionEvent_getHistoricalX(event, j, i),
                                AMotionEvent_getHistoricalY(event, j, i),
                                AMotionEvent_getPointerId(event, j));
                    }
                }
                for(int i = 0; i < AMotionEvent_getPointerCount(event); i++) {
                    eventDispatcher->touchMoved(
                            AMotionEvent_getX(event, i),
                            AMotionEvent_getY(event, i),
                            AMotionEvent_getPointerId(event, i));
                }
                break;


            case AMOTION_EVENT_ACTION_DOWN:
            case AMOTION_EVENT_ACTION_POINTER_DOWN:
                //LOGW("pointer down %d", touchId);
                eventDispatcher->touchDown(
                        AMotionEvent_getX(event, pointerIndex),
                        AMotionEvent_getY(event, pointerIndex),
                        AMotionEvent_getPointerId(event, pointerIndex));


                break;
            case AMOTION_EVENT_ACTION_UP:
            case AMOTION_EVENT_ACTION_POINTER_UP:
                //LOGW("pointer up %d", touchId);
                eventDispatcher->touchUp(
                        AMotionEvent_getX(event, pointerIndex),
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
    } else if(AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY) {
        int32_t keyAction = AKeyEvent_getAction(event);
        if(keyAction==AKEY_EVENT_ACTION_DOWN) {
            int32_t k = AKeyEvent_getKeyCode(event);
            int key = keycodeToKey(k);
            if(key!=0) {
                eventDispatcher->keyDown(key);
                return 1;
            }
            // if we don't recognize the key, return 0 for 'not handled'
            // it may be the volume controls and we want the OS to deal with that.
            return 0;
        } else if(keyAction==AKEY_EVENT_ACTION_UP) {
            int32_t k = AKeyEvent_getKeyCode(event);
            int key = keycodeToKey(k);
            if(key!=0) {
                eventDispatcher->keyUp(key);
                return 1;
            }
            // if we don't recognize the key, return 0 for 'not handled'
            // it may be the volume controls and we want the OS to deal with that.
            return 0;
        } else if(keyAction==AKEY_EVENT_ACTION_MULTIPLE) {
            Log::e() << "Unhandled AKEY_EVENT_ACTION_MULTIPLE";
            return 0;
        }
        return 1;
    }
    return 0; // event not handled
}

void clearUpGLResources() {
    //  Vbo::printVbos();
    Log::d() << "cleaning up GL resources";
    for(auto *vbo : Vbo::vbos) {
        vbo->clear();
        vbo->vertexArrayObject = 0;
    }

    for(auto *tex : Texture::textures) {
        tex->deallocate();
    }

    for(auto *font : Font::fonts) {
        font->clear();
    }

    for(auto *shader : Shader::shaders) {
        shader->shaderProgram = 0;
    }
}


bool ignoreNextGainedFocus = false;
bool clearedUpGLResources = false;
/**
 * Process the next main command.
 */
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {

    struct engine* engine = (struct engine*)app->userData;

    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            Log::d() << "APP_CMD_INIT_WINDOW";
            // The window is being shown, get it ready.
            if (engine->app->window != NULL) {
                engine_init_display(engine);
                engineReady = true;

                graphics.initGraphics();
               if(eventDispatcher!=nullptr && prepareFrame(engine)) {
//                    engine_draw_blankFrame(engine);
                   graphics.width = engine->width;
                   graphics.height = engine->height;
                    eventDispatcher->androidDrawLoading();
                    eglSwapBuffers(engine->display, engine->surface);
               }
                if(clearedUpGLResources && eventDispatcher!=nullptr) {

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
            clearUpGLResources();
            clearedUpGLResources = true;
            engine_term_display(engine);
            engineReady = false;
            break;
/*
        case APP_CMD_GAINED_FOCUS:
            Log::d() << "APP_CMD_GAINED_FOCUS";
            if(!ignoreNextGainedFocus) {
                if (eventDispatcher && eventDispatcher->isReady()) {
                    clearUpGLResources();
                    Log::d() << "allocating GL resources";
                    eventDispatcher->resized();
                }
            }
            ignoreNextGainedFocus = false;
            engineReady = true;
            break;
*/
        case APP_CMD_LOST_FOCUS:
            Log::d() << "APP_CMD_LOST_FOCUS";
            break;

        case APP_CMD_CONFIG_CHANGED:
            Log::e() << "APP_CMD_CONFIG_CHANGED";
            break;

        case APP_CMD_START:
            Log::d() << "APP_CMD_START";
            break;

        case APP_CMD_STOP:
            Log::d() << "APP_CMD_STOP";
            if(eventDispatcher && eventDispatcher->app!=nullptr) {
                eventDispatcher->didEnterBackground();
            }
            break;


        case APP_CMD_SAVE_STATE:
            Log::d() << "APP_CMD_SAVE_STATE";
            break;

        case APP_CMD_WINDOW_RESIZED:
            Log::e() << "APP_CMD_WINDOW_RESIZED";
            break;

        case APP_CMD_RESUME:
            Log::d() << "APP_CMD_RESUME";
            if(eventDispatcher && eventDispatcher->app!=nullptr) {
                eventDispatcher->androidOnResume();
            }
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
            if(eventDispatcher && eventDispatcher->app!=nullptr) {
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
/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app* state) {
    app = nullptr;
   eventDispatcher = nullptr;

    globalAppPtr = state;
    struct engine engine;

    memset(&engine, 0, sizeof(engine));
    state->userData = &engine;
    state->onAppCmd = engine_handle_cmd;
    state->onInputEvent = engine_handle_input;
    engine.app = state;

    app = instantiateApp(graphics);
    initMZGL(app);

    // loop waiting for stuff to do.
    while (1) {
        // Read all pending events.
        int ident;
        int events;
        struct android_poll_source* source;

        while ((ident=ALooper_pollAll(engineReady?0:-1, nullptr, &events,
                                      (void**)&source)) >= 0) {
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

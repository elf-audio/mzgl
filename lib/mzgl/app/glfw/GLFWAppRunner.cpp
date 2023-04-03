//
//  main.m
//  MZGL
//
//  Created by Marek Bereza on 15/01/2018.
//  Copyright © 2018 Marek Bereza. All rights reserved.
//

#ifdef _WIN32
#include <glew.h>
#endif
#ifdef __linux__
#include <gtk/gtk.h>
#endif
#include "GLFWAppRunner.h"
#include "glfw3.h"
#include <stdlib.h>
#include <stdio.h>
#include "filesystem.h"

#include "util.h"
#include "log.h"


using namespace std;

float mouseX = 0;
float mouseY = 0;

#define NUM_MOUSE_BUTTONS 8

std::vector<bool> buttons;
bool mouseIsDown = false;

bool leftAltDown = false;
bool rightAltDown = false;

bool leftShiftDown = false;
bool rightShiftDown = false;

bool framebuferResized = false;

EventDispatcher *getEventDispatcher(GLFWwindow *window) {
    auto *app = (GLFWAppRunner*)glfwGetWindowUserPointer(window);
    return app->eventDispatcher.get();
}

Graphics &getGraphics(GLFWwindow *window) {
    auto *app = (GLFWAppRunner*)glfwGetWindowUserPointer(window);
    return app->graphics;
}


static void error_callback(int error, const char* description) {
    fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    //    glfwSetWindowShouldClose(window, GLFW_TRUE);
    if(action==GLFW_PRESS || action==GLFW_REPEAT) {
        if(key==GLFW_KEY_LEFT_ALT) {
            leftAltDown = true;
        } else if(key==GLFW_KEY_RIGHT_ALT) {
            rightAltDown = true;
        } else if(key==GLFW_KEY_LEFT_SHIFT) {
            leftShiftDown = true;
        } else if(key==GLFW_KEY_RIGHT_SHIFT) {
            rightShiftDown = true;
        }
        getEventDispatcher(window)->keyDown(key);

    } else if(action==GLFW_RELEASE) {
        if(key==GLFW_KEY_LEFT_ALT) {
            leftAltDown = false;
        } else if(key==GLFW_KEY_RIGHT_ALT) {
            rightAltDown = false;
        } else if(key==GLFW_KEY_LEFT_SHIFT) {
            leftShiftDown = false;
        } else if(key==GLFW_KEY_RIGHT_SHIFT) {
            rightShiftDown = false;
        }
        getEventDispatcher(window)->keyUp(key);
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {

    if(action==GLFW_PRESS) {
        mouseIsDown = true;
        buttons[button] = true;
        getEventDispatcher(window)->touchDown(mouseX, mouseY, button);
    } else {
        getEventDispatcher(window)->touchUp(mouseX, mouseY, button);
        buttons[button] = false;
        for(const auto &b : buttons) {
            if(b) return;
        }
        mouseIsDown = false;
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    auto &g = getGraphics(window);
    mouseX = xpos;
    mouseY = ypos;
    if(!mouseIsDown) {
        getEventDispatcher(window)->touchOver(mouseX, mouseY);

    } else {
        for(int i = 0; i < buttons.size(); i++) {
            if(buttons[i]) {

                getEventDispatcher(window)->touchMoved(mouseX, mouseY, i);

            }
        }
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (leftAltDown || rightAltDown) {
        printf("yoffset: %.2f\n", yoffset);
        getEventDispatcher(window)->mouseZoomed(mouseX, mouseY, yoffset*-0.03);
    } else {
#ifdef WIN32
        // speed up windows scrolling.
        // should really have acceleration here...
        xoffset *= 3;
        yoffset *= 3;
#endif
        if(leftShiftDown || rightShiftDown) {
            std::swap(xoffset, yoffset);
        }
        getEventDispatcher(window)->mouseScrolled(mouseX, mouseY, xoffset, yoffset);
    }
}



void window_size_callback(GLFWwindow* window, int width, int height) {


    auto &g = getGraphics(window);
    glViewport(0, 0, width, height);

    g.width = width;
    g.height = height;

    framebuferResized = true; // just mark that we need to resize
    Log::d() << "resized to " << width << height;
}

void drop_callback(GLFWwindow* window, int count, const char** paths) {
    for (int i = 0; i < count; i++) {
        getEventDispatcher(window)->openUrl(paths[i]);
    }
}


float getMainMonitorScale() {
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if(monitor!=nullptr) {
        float xscale, yscale;
        glfwGetMonitorContentScale(monitor, &xscale, &yscale);
        Log::d() << "scale x: " << xscale << " scale y: " << yscale;
        return xscale;
    } else {
        printf("Error, can't find main monitor\n");

    }
    return 1;
}


void GLFWAppRunner::setCallbacks() {
    if(window==nullptr) {
        printf("ERROR! window not initialized yet!\n");
    }
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetScrollCallback(window, scroll_callback);
    //glfwSetWindowSizeCallback(window, window_size_callback);
    glfwSetFramebufferSizeCallback(window, window_size_callback);
    glfwSetDropCallback(window, drop_callback);
    glfwSetWindowUserPointer(window, this);

}


#ifdef METAL_BACKEND
// lifting from here: https://gist.github.com/gcatlin/987be74e2d58da96093a7598f3fbfb27
#	import <Metal/Metal.h>
#	import <QuartzCore/CAMetalLayer.h>
#endif

void GLFWAppRunner::run(int argc, char *argv[]) {


    loadCommandLineArgs(argc, (const char **)argv);

    for(int i =0; i < NUM_MOUSE_BUTTONS; i++) buttons.push_back(false);

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        printf("Can't init GLFW\n");
        exit(EXIT_FAILURE);
    }


    graphics.pixelScale = 1.0f; // actually we don't need any scaling, this is legacy code for compatibility

    glfwWindowHint(GLFW_SAMPLES, 16);

	
#ifdef METAL_BACKEND
	
	const id<MTLDevice> gpu = MTLCreateSystemDefaultDevice();
	const id<MTLCommandQueue> queue = [gpu newCommandQueue];
	CAMetalLayer *swapchain = [CAMetalLayer layer];
	swapchain.device = gpu;
	swapchain.opaque = YES;
	
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	
#else
#	ifdef UNIT_TEST
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#	else
    // this was set to 2.0 before, I bumped it to 3.2 so I can use imgui
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
#	endif
#endif

    int requestedWidth = -1;
    int requestedHeight = -1;

    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    if (primaryMonitor != nullptr) {
         const GLFWvidmode* currentVideoMode = glfwGetVideoMode(primaryMonitor);
         if (currentVideoMode != nullptr) {
            float h = (currentVideoMode->height) * 0.8; // make it 0.9 of max height, so there is room for decorations
            float w = h * 0.54; // set width as 0.54 of height, that looks OK
            requestedHeight = (int)h;
            requestedWidth = (int)w;
            if (requestedWidth > currentVideoMode->width) {
                requestedWidth = currentVideoMode->width; // clamp in case of pivoted monitor
            }
         }
    }

    // Note that graphics object is not fully functional here
    // as we need to update width/height later on.
    app = instantiateApp(graphics);

    if (requestedWidth != -1) {
        graphics.width = requestedWidth;
        graphics.height = requestedHeight;
    }

    Log::d() << "Request window " << (graphics.width) << "x"
             << (graphics.height);

    window = glfwCreateWindow(graphics.width, graphics.height, "mzgl", NULL, NULL);
	
#ifdef METAL_BACKEND
	NSWindow *nswindow = glfwGetCocoaWindow(window);
	nswindow.contentView.layer = swapchain;
	nswindow.contentView.wantsLayer = YES;
	MTLClearColor color = MTLClearColorMake(0, 0, 0, 1);
#endif
	
    int windowH, windowW;
    glfwGetWindowSize(window, &windowW, &windowH); // note that it can be DIFFERENT than requested
    Log::d() << "Window crated: " << windowW << "x" << windowH;

    // Use Frame Buffer pixel size as graphics context size
    // That would be compatible with GL functions using pixel coords (glViewport, glScissors, etc.)
    glfwGetFramebufferSize(window, &windowW, &windowH);
    Log::d() << "FB size: " << windowW << "x" << windowH;

    if (!window) {
        printf("Can't create GLFW window\n");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    graphics.width = windowW;
    graphics.height = windowH;

    app->windowHandle = window;

#ifdef __linux__
    gtk_init(&argc, &argv);
#endif

    if(argc>0) {
        glfwSetWindowTitle(window, fs::path(argv[0]).filename().string().c_str());
    }


    setCallbacks();

    // NOTE: OpenGL error checks have been omitted for brevity
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // GL_MULTISAMPLE does not seem to exist on certain linux environment.
    // e.g. https://stackoverflow.com/questions/4207506/where-is-gl-multisample-defined
#ifdef GL_MULTISAMPLE
    glEnable(GL_MULTISAMPLE);
#endif

#ifdef _WIN32
    GLenum err = glewInit();
    if(err!=GLEW_OK) {
        printf("Problem with glew\n");
        return;
    }
#endif


    initMZGL(app);

    eventDispatcher = std::make_shared<EventDispatcher>(app);
    eventDispatcher->setup();

    while (!glfwWindowShouldClose(window)) {

#ifdef METAL_BACKEND
		@autoreleasepool {
				   color.red = (color.red > 1.0) ? 0 : color.red + 0.01;

				   id<CAMetalDrawable> surface = [swapchain nextDrawable];

				   MTLRenderPassDescriptor *pass = [MTLRenderPassDescriptor renderPassDescriptor];
				   pass.colorAttachments[0].clearColor = color;
				   pass.colorAttachments[0].loadAction  = MTLLoadActionClear;
				   pass.colorAttachments[0].storeAction = MTLStoreActionStore;
				   pass.colorAttachments[0].texture = surface.texture;

				   id<MTLCommandBuffer> buffer = [queue commandBuffer];
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

//#ifdef _WIN32
//    delete app;
//#endif


//	exit(EXIT_SUCCESS);
}

void GLFWAppRunner::stop() {
    glfwSetWindowShouldClose(window, true);
}



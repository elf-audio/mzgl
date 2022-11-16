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

EventDispatcher *getEventDispatcher(GLFWwindow *window) {
	auto *app = (GLFWAppRunner*)glfwGetWindowUserPointer(window);
	return app->eventDispatcher;
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
	mouseX = xpos*g.pixelScale;
	mouseY = ypos*g.pixelScale;
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

	g.width = width*g.pixelScale;
	g.height = height*g.pixelScale;

    getEventDispatcher(window)->resized();
	
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
	glfwSetWindowSizeCallback(window, window_size_callback);
	glfwSetDropCallback(window, drop_callback);
	glfwSetWindowUserPointer(window, this);

}



void GLFWAppRunner::run(int argc, char *argv[]) {


	loadCommandLineArgs(argc, (const char **)argv);

	for(int i =0; i < NUM_MOUSE_BUTTONS; i++) buttons.push_back(false);
	
	glfwSetErrorCallback(error_callback);
	if (!glfwInit()) {
		printf("Can't init GLFW\n");
		exit(EXIT_FAILURE);
	}


	graphics.pixelScale = getMainMonitorScale();

    Log::d() << "Pixel scale is" << graphics.pixelScale;

    app = instantiateApp(graphics);

    // on linux window is really small, so lets bump it up.
    graphics.width *= graphics.pixelScale;
    graphics.height *= graphics.pixelScale;
	
	glfwWindowHint(GLFW_SAMPLES, 16);

#ifdef UNIT_TEST
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#else
    // this was set to 2.0 before, I bumped it to 3.2 so I can use imgui
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
#endif

    window = glfwCreateWindow(graphics.width/graphics.pixelScale, graphics.height/graphics.pixelScale, "mzgl", NULL, NULL);

    if (!window) {
		printf("Can't create GLFW window\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	app->windowHandle = window;

#ifdef __linux__
    gtk_init(&argc, &argv);

    if(argc>0) {
        glfwSetWindowTitle(window, fs::path(argv[0]).filename().string().c_str());
    }
#endif

	setCallbacks();
	
	glfwSwapInterval(1);
	// NOTE: OpenGL error checks have been omitted for brevity
	glfwMakeContextCurrent(window);

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

	eventDispatcher = new EventDispatcher(app);
	eventDispatcher->setup();

	while (!glfwWindowShouldClose(window)) {

		eventDispatcher->runFrame();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	eventDispatcher->exit();

	glfwDestroyWindow(window);
	glfwTerminate();
//	exit(EXIT_SUCCESS);
}

void GLFWAppRunner::stop() {
	glfwSetWindowShouldClose(window, true);
}



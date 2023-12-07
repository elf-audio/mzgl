
#include "App.h"

/**
 * Every executable has a subclass of App, which is the main class.
 */
class MyApp : public App {
public:
	// in constructor you can allocate things but don't do anything
	// with GPU/OpenGL stuff because the window/opengl context hasn't
	// been created yet.
	MyApp(Graphics &g)
		: App(g) {}

	/////////////////////////////////////////////////////////////
	// LIFE-CYCLE

	// called after app is constructed and ready to draw
	// e.g. you can allocate GPU primitives now
	void setup() override { printf("Setup called\n"); }

	// called before draw() and you can do any non graphical things here
	// - it uses the time available in between frames.
	void update() override {}

	void draw() override {
		// set background colour to dark red
		g.clear(0.2, 0, 0);

		// set current colour to white
		g.setColor(1);

		// draw some text
		g.drawTextCentred("move your mouse to see events in the terminal", vec2(g.width / 2, g.height / 2));
	}

	// called just before the program quits - this is a time to clear
	// everything up.
	void exit() override { printf("exit called\n"); }

	/////////////////////////////////////////////////////////////
	// TOUCHES
	// these are also triggered on mouse events (id is the persistent touch id
	// to help you identify which touch is which.)

	// touchOver doesn't really exist for touch devices but its
	// a way for mouse over inputs to be received (i.e.) when you have
	// a mouse hovering over an element rather than clicking
	void touchOver(float x, float y) override { printf("Touch over: x: %.0f, y: %.0f\n", x, y); }

	// when the touch actually touches the screen this is called
	// (or when mouse is clicked)
	void touchDown(float x, float y, int id) override {
		printf("Touch down: x: %.0f, y: %.0f, id: %d\n", x, y, id);
	}

	// this happens continually whilst a finger or mouse is being
	// dragged across the screen
	void touchMoved(float x, float y, int id) override {
		printf("Touch moved: x: %.0f, y: %.0f, id: %d\n", x, y, id);
	}

	// this is when the touch comes off the screen or the mouse
	// button is released
	void touchUp(float x, float y, int id) override { printf("Touch up: x: %.0f, y: %.0f, id: %d\n", x, y, id); }

	/////////////////////////////////////////////////////////////
	// KEY EVENTS

	void keyDown(int key) override { printf("key down %c\n", key); }

	void keyUp(int key) override { printf("key up %c\n", key); }

	/////////////////////////////////////////////////////////////
	// WINDOW EVENTS

	void resized() override {
		// when resized gets called, it will have already set
		// g.width and g.height
		printf("new window size: %d x %d\n", g.width, g.height);
	}
};

// every executable has instantiateApp as an entry point
// this is called by main() which is inside the mzgl library.
//
// The parameter 'g' is the graphics context, of which there
// must be only one per app. App needs it in its constructo
std::shared_ptr<App> instantiateApp(Graphics &g) {
	// how to get command line args:
	auto commandLineArgs = getCommandLineArgs();
	for (const auto &s: commandLineArgs) {
		printf("> %s\n", s.c_str());
	}

	// set the window width and height on the graphics class.
	g.width	 = 800;
	g.height = 512;

	return std::make_shared<MyApp>(g);
}
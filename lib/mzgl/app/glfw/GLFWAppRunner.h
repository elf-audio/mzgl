
struct GLFWwindow;

#include "EventDispatcher.h"

class GLFWAppRunner {
public:
	void run(int argc, char *argv[]);
	void stop();
	App *app = nullptr;
	EventDispatcher *eventDispatcher = nullptr;
	Graphics graphics;
	GLFWwindow *window = nullptr;
private:
	void setCallbacks();
};

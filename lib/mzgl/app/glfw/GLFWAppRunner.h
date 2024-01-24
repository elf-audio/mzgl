
struct GLFWwindow;

#include <mzgl/util/EventDispatcher.h>

class GLFWAppRunner {
public:
	void run(int argc, char *argv[]);
	void stop();
	std::shared_ptr<App> app						 = nullptr;
	std::shared_ptr<EventDispatcher> eventDispatcher = nullptr;
	Graphics graphics;
	GLFWwindow *window = nullptr;

private:
	void setCallbacks();
};

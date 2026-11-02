#pragma once

#include <functional>
#include <memory>

struct GLFWwindow;

// GLFW's Wayland backend ignores wl_touch, so touchscreens do nothing there (under X11
// the server emulates the pointer instead). This binds the seat(s) itself on GLFW's
// wl_display and reports touches on the window's surface. Its proxies live on the
// default queue, so glfwPollEvents()/glfwWaitEvents() dispatch them on the main thread.
class WaylandTouch {
public:
	enum class Phase { down, moved, up };

	// x, y are surface-local (window) coordinates, like GLFW's cursor position
	using Callback = std::function<void(Phase phase, int id, double x, double y)>;

	// Returns nullptr unless GLFW is running on the Wayland platform.
	static auto create(GLFWwindow *window, Callback callback) -> std::unique_ptr<WaylandTouch>;

	~WaylandTouch();

	struct Impl;

private:
	explicit WaylandTouch(std::unique_ptr<Impl> impl);
	std::unique_ptr<Impl> impl;
};

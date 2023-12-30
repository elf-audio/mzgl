#include "DesktopWindowFileDragHandler.h"
#include "EventDispatcher.h"
#include "GLFWAppRunner.h"
#include <GLFW/glfw3.h>

namespace file_drag_handler {

	static auto getEventDispatcher(GLFWwindow *window) -> EventDispatcher * {
		const auto app {reinterpret_cast<GLFWAppRunner *>(glfwGetWindowUserPointer(window))};
		return app->eventDispatcher.get();
	}

	static auto makePathsVector(int count, const char **paths) {
		std::vector<ScopedUrlRef> out;
		out.reserve(count);
		for (int i = 0; i < count; i++) {
			out.push_back(ScopedUrl::create(paths[i]));
		}
		return out;
	}

	// For Linux we currently just set the drop callback using GLFW, which is only called
	// when the user releases the mouse button to drop files over the window.
	//
	// There won't be visual feedback when the user is dragging files over the window
	// with the button still pressed.
	//
	// To fix that we would need to do something platform-specific here and call these
	// Listener functions at the appropriate times:
	//	- Listener::onDragBegin
	//	- Listener::onDragCancel
	//	- Listener::onDrag
	//	- Listener::onDrop
	//
	// See the Windows implementation for an example of this.

	static auto callback(GLFWwindow *window, int count, const char **paths) -> void {
		getEventDispatcher(window)->filesDropped(makePathsVector(count, paths), 0, [] {});
	}

	struct Handler {
		GLFWwindow *window;
	};

	auto Deleter::operator()(Handler *ptr) const -> void {
		glfwSetDropCallback(ptr->window, nullptr);
		delete ptr;
	}

	auto init(GLFWwindow *window, Listener::Ptr listener) -> Ptr {
		auto out {std::unique_ptr<Handler, Deleter>(new Handler {}, Deleter {})};
		out->window = window;
		glfwSetDropCallback(window, callback);
		return out;
	}

} // namespace file_drag_handler

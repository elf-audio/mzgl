#include "DesktopWindowFileDragHandler.h"

// Emscripten's GLFW port has no file drag/drop support. No-op stub.

namespace file_drag_handler {

	struct Handler {};

	auto Deleter::operator()(Handler *ptr) const -> void { delete ptr; }

	auto init(GLFWwindow *window, Listener::Ptr listener) -> Ptr {
		return std::unique_ptr<Handler, Deleter>(new Handler {}, Deleter {});
	}

} // namespace file_drag_handler

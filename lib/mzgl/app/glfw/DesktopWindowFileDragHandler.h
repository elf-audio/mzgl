#pragma once

#include <memory>
#include <string>
#include <vector>
#include "EventDispatcher.h"

struct GLFWwindow;

namespace file_drag_handler {

struct Listener {
	using Ptr = std::unique_ptr<Listener>;
	virtual ~Listener() {}
	virtual auto onDragBegin(std::vector<std::string> paths, int x, int y) -> void = 0;
	virtual auto onDragCancel() -> void = 0;
	virtual auto onDrag(int x, int y) -> void = 0;
	virtual auto onDrop(int x, int y) -> void = 0;
};

struct Handler;
struct Deleter { auto operator()(Handler* ptr) const -> void; };
using Ptr = std::unique_ptr<Handler, Deleter>;

auto init(GLFWwindow* window, Listener::Ptr listener) -> Ptr;

inline auto makeFileDragListener(EventDispatcher* eventDispatcher) {
	struct Listener : file_drag_handler::Listener {
		Listener(EventDispatcher* eventDispatcher)
			: eventDispatcher_{eventDispatcher}
		{
		}
		auto onDragBegin(std::vector<std::string> paths, int x, int y) -> void override {
			filePaths_ = std::move(paths);
			eventDispatcher_->fileDragUpdate(static_cast<float>(x), static_cast<float>(y), 0, filePaths_.size());
		}
		auto onDragCancel() -> void override {
			eventDispatcher_->fileDragCancelled(0);
		}
		auto onDrag(int x, int y) -> void override {
			eventDispatcher_->fileDragUpdate(static_cast<float>(x), static_cast<float>(y), 0, filePaths_.size());
		}
		auto onDrop(int x, int y) -> void override {
			eventDispatcher_->filesDropped(filePaths_, 0, []{});
		}
	private:
		EventDispatcher* eventDispatcher_;
		std::vector<std::string> filePaths_;
	};
	return std::make_unique<Listener>(eventDispatcher);
}

} // file_drag_handler

struct DesktopWindowFileDragHandler {
	using Listener = file_drag_handler::Listener;
	DesktopWindowFileDragHandler(GLFWwindow* window, Listener::Ptr listener)
		: impl_{file_drag_handler::init(window, std::move(listener))}
	{
	}
private:
	file_drag_handler::Ptr impl_;
};

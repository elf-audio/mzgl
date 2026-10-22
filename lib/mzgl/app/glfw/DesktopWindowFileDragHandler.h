#pragma once

#include <functional>
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
		virtual auto onDragCancel() -> void											   = 0;
		virtual auto onDrag(int x, int y) -> void									   = 0;
		virtual auto onDrop(int x, int y) -> void									   = 0;
	};

	struct Handler;
	struct Deleter {
		auto operator()(Handler *ptr) const -> void;
	};
	using Ptr = std::unique_ptr<Handler, Deleter>;

	auto init(GLFWwindow *window, Listener::Ptr listener) -> Ptr;

	// Windows only: register the drop target on a native HWND that isn't a GLFW
	// window (the VST3 editor's child window). Returns null if OLE / the
	// registration fails; the error is logged.
	auto initWithNativeWindow(void *hwnd, Listener::Ptr listener) -> Ptr;

	// Windows only. A drag-out (startNativeFileDrag() in util.h) is queued, not
	// started on the spot: the caller is deep inside an event dispatch and
	// DoDragDrop runs a nested message loop until the drop. The window's owner
	// calls this from its loop at a point with no frame or dispatch in
	// progress, and the queued drag runs there. `frame` (may be null) is called
	// at ~60Hz for as long as the drag lasts so the window keeps drawing; the
	// VST3 editor passes null since its WM_TIMER frames keep firing anyway.
	void runPendingNativeDrag(const std::function<void()> &frame);

	inline auto makeFileDragListener(EventDispatcher *eventDispatcher) {
		struct Listener : file_drag_handler::Listener {
			Listener(EventDispatcher *eventDispatcher)
				: eventDispatcher_ {eventDispatcher} {}
			auto onDragBegin(std::vector<std::string> paths, int x, int y) -> void override {
				filePaths_ = std::move(paths);
				eventDispatcher_->fileDragUpdate(
					static_cast<float>(x), static_cast<float>(y), 0, filePaths_.size());
			}
			auto onDragCancel() -> void override { eventDispatcher_->fileDragCancelled(0); }
			auto onDrag(int x, int y) -> void override {
				eventDispatcher_->fileDragUpdate(
					static_cast<float>(x), static_cast<float>(y), 0, filePaths_.size());
			}
			auto onDrop(int x, int y) -> void override {
				std::vector<ScopedUrlRef> paths;
				for (auto &path: filePaths_) {
					paths.push_back(ScopedUrl::create(path));
				}
				eventDispatcher_->filesDropped(paths, 0);
			}

		private:
			EventDispatcher *eventDispatcher_;
			std::vector<std::string> filePaths_;
		};
		return std::make_unique<Listener>(eventDispatcher);
	}

} // namespace file_drag_handler

struct DesktopWindowFileDragHandler {
	using Listener = file_drag_handler::Listener;
	DesktopWindowFileDragHandler(GLFWwindow *window, Listener::Ptr listener)
		: impl_ {file_drag_handler::init(window, std::move(listener))} {}

private:
	file_drag_handler::Ptr impl_;
};

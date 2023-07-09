#include "DesktopWindowEventHandler.h"
#include "EventDispatcher.h"

auto DesktopWindowEventHandler::isAnyMouseButtonDown() const -> bool {
    for (const auto b : buttons_) {
        if (b) {
            return true;
		}
	}
    return false;
}

auto DesktopWindowEventHandler::isDown(Modifier modifier) const -> bool {
	if (modifier == Modifier::none) {
		return false;
	}
	return modifiers_[size_t(modifier)];
}

auto DesktopWindowEventHandler::cursorPos(EventDispatcher* eventDispatcher, float x, float y) -> void {
    mouseX_ = x;
    mouseY_ = y;
    if (!isAnyMouseButtonDown()) {
		eventDispatcher->touchOver(mouseX_, mouseY_);
		return;
	}
	for(int i = 0; i < buttons_.size(); i++) {
		if(buttons_[i]) {
			eventDispatcher->touchMoved(mouseX_, mouseY_, i);
		}
	}
}

auto DesktopWindowEventHandler::filesDropped(EventDispatcher* eventDispatcher, std::vector<std::string> paths) -> void {
	// FIXME: This isn't currently good enough
	// See https://github.com/elf-audio/koala/issues/117
	eventDispatcher->filesDropped(std::move(paths), -1, []{});
}

auto DesktopWindowEventHandler::getModifier(int key) const -> Modifier {
	switch (key) {
		case GLFW_KEY_LEFT_ALT: return Modifier::left_alt;
		case GLFW_KEY_RIGHT_ALT: return Modifier ::right_alt;
		case GLFW_KEY_LEFT_SHIFT: return Modifier::left_shift;
		case GLFW_KEY_RIGHT_SHIFT: return Modifier ::right_shift;
	}
	return Modifier::none;
}

auto DesktopWindowEventHandler::key(EventDispatcher* eventDispatcher, int key, int action) -> void {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		keyDown(eventDispatcher, key);
		return;
    }
    if (action == GLFW_RELEASE) {
		keyUp(eventDispatcher, key);
		return;
    }
}

auto DesktopWindowEventHandler::keyDown(EventDispatcher* eventDispatcher, int key) -> void {
	const auto modifier{getModifier(key)};
	eventDispatcher->keyDown(key);
	if (modifier != Modifier::none) {
		modifiers_[size_t(modifier)] = true;
	}
}

auto DesktopWindowEventHandler::keyUp(EventDispatcher* eventDispatcher, int key) -> void {
	const auto modifier{getModifier(key)};
	eventDispatcher->keyUp(key);
	if (modifier != Modifier::none) {
		modifiers_[size_t(modifier)] = false;
	}
}

auto DesktopWindowEventHandler::mouseButton(EventDispatcher* eventDispatcher, int button, int action) -> void {
    if (action == GLFW_PRESS) {
        buttons_[button] = true;
        eventDispatcher->touchDown(mouseX_, mouseY_, button);
        return;
	}
	// Check if we are aware of any mouse button being down at this point.
	// Otherwise touchUp() can crash if there was no corresponding
	// touchDown() event.
	// This can occur if the window is a child window (e.g. in the VST
	// version, and the mouse button was pressed while the cursor was
	// outside the bounds of the child window, but then released while
	// the cursor is inside the child window.
	if (isAnyMouseButtonDown()) {
		eventDispatcher->touchUp(mouseX_, mouseY_, button);
	}
	buttons_[button] = false;
}

auto DesktopWindowEventHandler::scroll(EventDispatcher* eventDispatcher, float x, float y) -> void {
	if (isDown(Modifier::left_alt) || isDown(Modifier::right_alt)) {
        eventDispatcher->mouseZoomed(mouseX_, mouseY_, y * -0.03);
		return;
    }
#ifdef WIN32
	// speed up windows scrolling.
	// should really have acceleration here...
	x *= 3;
	y *= 3;
#endif
	if (isDown(Modifier::left_shift) || isDown(Modifier::right_shift)) {
		std::swap(x, y);
	}
	eventDispatcher->mouseScrolled(mouseX_, mouseY_, x, y);
}

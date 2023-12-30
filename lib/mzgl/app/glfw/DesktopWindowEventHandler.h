#pragma once

#include <array>
#include <string>
#include <vector>
#include "ScopedUrl.h"

class EventDispatcher;

struct DesktopWindowEventHandler {
	auto cursorPos(EventDispatcher *eventDispatcher, float x, float y) -> void;
	auto filesDropped(EventDispatcher *eventDispatcher, std::vector<ScopedUrlRef> paths) -> void;
	auto key(EventDispatcher *eventDispatcher, int key, int action) -> void;
	auto keyDown(EventDispatcher *eventDispatcher, int key) -> void;
	auto keyUp(EventDispatcher *eventDispatcher, int key) -> void;
	auto mouseButton(EventDispatcher *eventDispatcher, int button, int action) -> void;
	auto scroll(EventDispatcher *eventDispatcher, float x, float y) -> void;

private:
	enum class Modifier { none = -1, left_alt, right_alt, left_shift, right_shift };
	auto getModifier(int key) const -> Modifier;
	auto isAnyMouseButtonDown() const -> bool;
	auto isDown(Modifier modifier) const -> bool;
	float mouseX_ {0};
	float mouseY_ {0};
	std::array<bool, 8> buttons_ {false};
	std::array<bool, 4> modifiers_ {false};
};
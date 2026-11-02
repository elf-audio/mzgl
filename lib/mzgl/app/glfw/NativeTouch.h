#pragma once

#include <functional>

// Shared shape of the platform touch sources the GLFW runner wires up itself,
// because GLFW reports at most one emulated pointer: WaylandTouch (GLFW ignores
// wl_touch) and Win32Touch (Windows' mouse emulation of touch). Each reports
// every finger with its own id; GLFWAppRunner maps them into the dispatcher.
namespace NativeTouch {
	enum class Phase { down, moved, up };

	// x, y are window coordinates, like GLFW's cursor position
	using Callback = std::function<void(Phase phase, int id, double x, double y)>;
} // namespace NativeTouch

#pragma once

// The Xlib/EGL half of the Linux plugin view. Xlib's global-scope typedefs
// (Font, Cursor) collide with mzgl's types, so no translation unit can hold
// both: X11Native.cpp includes Xlib/EGL and nothing of mzgl, X11PluginView.cpp
// includes mzgl and talks to this class through plain integers and the neutral
// event struct below. Window ids are uint64_t, keys are raw X11 keysyms
// (protocol constants, stable - the mzgl side maps them to MZ_KEY_*).

#include <cstdint>
#include <memory>
#include <string>

namespace mzgl {

	struct X11NativeEvent {
		// Names deliberately differ from X11's event macros (None, Expose,
		// FocusIn...) so the Xlib-including TU can use these without clashes.
		enum class Type {
			NoEvent,
			Exposed,
			FocusGained,
			FocusLost,
			Motion,
			ButtonDown, // button 1/2/3
			ButtonUp,
			Scroll, // scrollX/scrollY in notches
			KeyDown, // keysym (+ textUtf8 when the input method produced text)
			KeyUp,
		};
		Type type {Type::NoEvent};
		float x {0};
		float y {0};
		int button {0};
		float scrollX {0};
		float scrollY {0};
		bool shift {false};
		bool alt {false};
		unsigned long keysym {0};
		std::string textUtf8;
	};

	class X11Native {
	public:
		X11Native();
		~X11Native();

		X11Native(const X11Native &)			= delete;
		X11Native &operator=(const X11Native &) = delete;

		// Child window + EGL surface/context on the host's window, made current.
		// sampleCount() reports the MSAA samples the config actually got (0 or 4).
		bool create(uint64_t parentWindowId, int widthPx, int heightPx);
		void destroy();
		bool created() const;
		uint64_t windowId() const;
		int sampleCount() const;

		bool makeCurrent();
		void swapBuffers();

		void resizeWindow(int widthPx, int heightPx);
		// true = take keyboard focus (a text field is active), false = hand it
		// back to the parent so host shortcuts keep working.
		void setInputFocus(bool takeIt);

		// The shared X connection's fd (one connection serves every instance).
		static int connectionFd();
		// Drain one buffered event; false when none left. windowId identifies
		// which instance's window it belongs to.
		static bool nextEvent(uint64_t &windowId, X11NativeEvent &event);
		// Xft.dpi / GDK_SCALE, else 1.0.
		static float detectPixelScale();

	private:
		struct Impl;
		std::unique_ptr<Impl> impl;
	};

} // namespace mzgl

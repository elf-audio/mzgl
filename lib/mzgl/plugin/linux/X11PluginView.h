#pragma once

// The shared X11 + EGL/GLES3 editor embedding used by every Linux plugin
// format: the VST3 view (MzglVST3View_linux.cpp) and the CLAP gui adapt this
// one class to their host APIs. It owns the whole editor stack - child X11
// window on the host-provided parent, EGL context, Plugin/PluginEditor/
// Graphics/EventDispatcher - mirroring what MzglVST3View_win.cpp's Impl does
// with an HWND and D3D11.
//
// Threading contract: every method is called from the thread that drives the
// host's run loop (the VST3/CLAP main thread). The view opens its own X
// Display connection, so it never touches the host's Xlib state; the fd of
// that connection is what the adapter registers with the host's run loop
// (VST3 IRunLoop / CLAP posix-fd-support), and a ~16ms host timer drives
// renderFrame(). The view itself never creates threads or timers.
//
// This header is deliberately Xlib-free (window ids are uint64_t, all Xlib
// and EGL types live in the .cpp's Impl) so including it never leaks X11's
// Font/Cursor/None macros into mzgl code - see cmake/pch.cmake in koala for
// the collision this avoids.

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

class Plugin;

namespace mzgl {

	class X11PluginView {
	public:
		struct Setup {
			int widthPx		 = 1024;
			int heightPx	 = 300;
			float pixelScale = 1.f;
			// setDataPath() target (the bundle's Resources/data); empty leaves the
			// process data path alone.
			std::string dataDir;
			// Borrow the controller's plugin so editor knobs and host automation
			// share one instance; returning null falls back to instantiatePlugin().
			std::function<std::shared_ptr<Plugin>()> getPlugin;
		};

		X11PluginView();
		~X11PluginView();

		X11PluginView(const X11PluginView &)			= delete;
		X11PluginView &operator=(const X11PluginView &) = delete;

		// Create the child window on the host's X11 window id, the GL context and
		// the editor. false = nothing was created (no display, no EGL, ...).
		bool attach(uint64_t parentWindowId, const Setup &setup);
		// Tear everything down (idempotent; also run by the destructor).
		void detach();
		bool attached() const;

		// The X connection's fd, for the host run loop's event handler. -1 when
		// not attached.
		int connectionFd() const;
		// Drain and dispatch pending X events (input translation). Call from the
		// host's fd handler AND from the timer tick: X events can be sitting in
		// Xlib's buffer with the fd already read.
		void handleXEvents();
		// Render one frame (the first one runs mzgl init + the editor's setup()).
		// Call from the host's ~16ms timer.
		void renderFrame();

		// Host-driven resize, in physical pixels.
		void resize(int widthPx, int heightPx);
		// DPI scale change (host told us, or a monitor change): redraws the UI at
		// the new scale. Does not resize the window.
		void setPixelScale(float scale);

		// Best guess at the display's scale (Xft.dpi, then GDK_SCALE, else 1.0),
		// for hosts that never call IPlugViewContentScaleSupport/set_scale.
		static float detectPixelScale();

	private:
		struct Impl;
		std::unique_ptr<Impl> impl;
	};

} // namespace mzgl

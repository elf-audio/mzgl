// Xlib + EGL primitives for the Linux plugin view. This TU includes NOTHING
// of mzgl (see X11Native.h for why); logging goes through stderr.
#if defined(__linux__) && !defined(__ANDROID__)

#	include "X11Native.h"

#	include <X11/Xlib.h>
#	include <X11/Xutil.h>
#	include <X11/keysym.h>
#	include <EGL/egl.h>

#	include <cstdio>
#	include <cstdlib>
#	include <cstring>
#	include <map>

namespace mzgl {

	namespace {

		void logError(const char *msg) {
			fprintf(stderr, "[error] X11Native: %s\n", msg);
		}

		// One X connection + EGL display + share-root GL context per process,
		// never torn down - the same scheme as the Windows view's SharedGpu.
		// Per-instance contexts share the root's object space because mzgl caches
		// GL handles (shaders, fonts) in per-process statics that would dangle
		// across unshared contexts.
		struct SharedX11 {
			Display *display {nullptr};
			EGLDisplay eglDisplay {EGL_NO_DISPLAY};
			EGLConfig eglConfig {nullptr};
			EGLContext rootContext {EGL_NO_CONTEXT};
			XIM xim {nullptr};
			int sampleCount {0};
			bool failed {false};

			static SharedX11 &get() {
				static SharedX11 shared;
				return shared;
			}

			bool ensure() {
				if (display != nullptr || failed) return !failed;
				XInitThreads(); // cheap insurance: some hosts use Xlib from threads
				display = XOpenDisplay(nullptr);
				if (display == nullptr) {
					logError("XOpenDisplay failed (no DISPLAY?)");
					failed = true;
					return false;
				}
				eglDisplay = eglGetDisplay((EGLNativeDisplayType) display);
				if (eglDisplay == EGL_NO_DISPLAY || eglInitialize(eglDisplay, nullptr, nullptr) != EGL_TRUE) {
					logError("eglInitialize failed");
					eglDisplay = EGL_NO_DISPLAY;
					failed	   = true;
					return false;
				}
				eglBindAPI(EGL_OPENGL_ES_API);

				// RGBA8 window config, 4x MSAA to match the standalone's
				// GLFW_SAMPLES; fall back to no MSAA (llvmpipe and some Nvidia
				// EGL/X11 stacks have no multisampled window configs).
				for (int samples: {4, 0}) {
					const EGLint attribs[] = {
						// clang-format off
					EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
					EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
					EGL_RED_SIZE, 8,
					EGL_GREEN_SIZE, 8,
					EGL_BLUE_SIZE, 8,
					EGL_ALPHA_SIZE, 8,
					EGL_DEPTH_SIZE, 0,
					EGL_SAMPLE_BUFFERS, samples > 0 ? 1 : 0,
					EGL_SAMPLES, samples,
					EGL_NONE,
						// clang-format on
					};
					EGLint numConfigs = 0;
					if (eglChooseConfig(eglDisplay, attribs, &eglConfig, 1, &numConfigs) == EGL_TRUE
						&& numConfigs > 0) {
						sampleCount = samples;
						break;
					}
					eglConfig = nullptr;
				}
				if (eglConfig == nullptr) {
					logError("no usable EGL config");
					failed = true;
					return false;
				}

				const EGLint ctxAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
				rootContext				  = eglCreateContext(eglDisplay, eglConfig, EGL_NO_CONTEXT, ctxAttribs);
				if (rootContext == EGL_NO_CONTEXT) {
					logError("eglCreateContext (share root) failed");
					failed = true;
					return false;
				}

				// Input method for text fields; nullptr is survivable (latin-1
				// fallback via XLookupString).
				xim = XOpenIM(display, nullptr, nullptr, nullptr);
				return true;
			}
		};

	
	// window -> its input context, so the static nextEvent() can run the
	// right Xutf8LookupString for whichever instance the KeyPress targets.
	std::map<Window, XIC> &xicRegistry() {
		static std::map<Window, XIC> xics;
		return xics;
	}

} // namespace

	struct X11Native::Impl {
		Window parent {0};
		Window window {0};
		Colormap colormap {0};
		EGLSurface surface {EGL_NO_SURFACE};
		EGLContext context {EGL_NO_CONTEXT};
		XIC xic {nullptr};

		void destroy() {
			auto &shared = SharedX11::get();
			if (shared.eglDisplay != EGL_NO_DISPLAY) {
				eglMakeCurrent(shared.eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
				if (context != EGL_NO_CONTEXT) {
					eglDestroyContext(shared.eglDisplay, context);
					context = EGL_NO_CONTEXT;
				}
				if (surface != EGL_NO_SURFACE) {
					eglDestroySurface(shared.eglDisplay, surface);
					surface = EGL_NO_SURFACE;
				}
			}
			if (xic != nullptr) {
				if (window != 0) xicRegistry().erase(window);
				XDestroyIC(xic);
				xic = nullptr;
			}
			if (window != 0 && shared.display != nullptr) {
				XDestroyWindow(shared.display, window);
				if (colormap != 0) XFreeColormap(shared.display, colormap);
				XFlush(shared.display);
			}
			window	 = 0;
			colormap = 0;
			parent	 = 0;
		}
	};

	X11Native::X11Native()
		: impl(std::make_unique<Impl>()) {
	}

	X11Native::~X11Native() {
		destroy();
	}

	bool X11Native::create(uint64_t parentWindowId, int widthPx, int heightPx) {
		if (impl->window != 0) return true;
		auto &shared = SharedX11::get();
		if (!shared.ensure()) return false;

		impl->parent = (Window) parentWindowId;

		// The window's visual must match the EGL config.
		EGLint visualId = 0;
		eglGetConfigAttrib(shared.eglDisplay, shared.eglConfig, EGL_NATIVE_VISUAL_ID, &visualId);
		XVisualInfo visualTemplate {};
		visualTemplate.visualid = (VisualID) visualId;
		int visualCount			= 0;
		XVisualInfo *visual		= XGetVisualInfo(shared.display, VisualIDMask, &visualTemplate, &visualCount);
		if (visual == nullptr || visualCount == 0) {
			logError("no XVisualInfo for the EGL config's visual");
			impl->parent = 0;
			return false;
		}

		impl->colormap = XCreateColormap(shared.display, impl->parent, visual->visual, AllocNone);
		XSetWindowAttributes attrs {};
		attrs.colormap	   = impl->colormap;
		attrs.event_mask   = ExposureMask | StructureNotifyMask | ButtonPressMask | ButtonReleaseMask
							 | PointerMotionMask | KeyPressMask | KeyReleaseMask | FocusChangeMask;
		attrs.border_pixel = 0;
		impl->window	   = XCreateWindow(shared.display,
										   impl->parent,
										   0,
										   0,
										   (unsigned) widthPx,
										   (unsigned) heightPx,
										   0,
										   visual->depth,
										   InputOutput,
										   visual->visual,
										   CWColormap | CWEventMask | CWBorderPixel,
										   &attrs);
		XFree(visual);
		if (impl->window == 0) {
			logError("XCreateWindow failed");
			destroy();
			return false;
		}
		XMapWindow(shared.display, impl->window);

		impl->surface = eglCreateWindowSurface(
			shared.eglDisplay, shared.eglConfig, (EGLNativeWindowType) impl->window, nullptr);
		const EGLint ctxAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
		impl->context = eglCreateContext(shared.eglDisplay, shared.eglConfig, shared.rootContext, ctxAttribs);
		if (impl->surface == EGL_NO_SURFACE || impl->context == EGL_NO_CONTEXT || !makeCurrent()) {
			logError("EGL surface/context creation failed");
			destroy();
			return false;
		}
		// Never block the host's UI thread waiting for vsync - same reasoning as
		// Present(0) in the Windows view.
		eglSwapInterval(shared.eglDisplay, 0);

		if (shared.xim != nullptr) {
			impl->xic = XCreateIC(shared.xim,
								  XNInputStyle,
								  XIMPreeditNothing | XIMStatusNothing,
								  XNClientWindow,
								  impl->window,
								  nullptr);
								 if (impl->xic != nullptr) xicRegistry()[impl->window] = impl->xic;
		}

		XFlush(shared.display);
		return true;
	}

	void X11Native::destroy() {
		if (impl) impl->destroy();
	}

	bool X11Native::created() const {
		return impl->window != 0;
	}

	uint64_t X11Native::windowId() const {
		return (uint64_t) impl->window;
	}

	int X11Native::sampleCount() const {
		return SharedX11::get().sampleCount;
	}

	bool X11Native::makeCurrent() {
		auto &shared = SharedX11::get();
		if (impl->surface == EGL_NO_SURFACE) return false;
		return eglMakeCurrent(shared.eglDisplay, impl->surface, impl->surface, impl->context) == EGL_TRUE;
	}

	void X11Native::swapBuffers() {
		auto &shared = SharedX11::get();
		if (impl->surface != EGL_NO_SURFACE) {
			eglSwapBuffers(shared.eglDisplay, impl->surface);
		}
	}

	void X11Native::resizeWindow(int widthPx, int heightPx) {
		auto &shared = SharedX11::get();
		if (impl->window == 0 || widthPx <= 0 || heightPx <= 0) return;
		XResizeWindow(shared.display, impl->window, (unsigned) widthPx, (unsigned) heightPx);
		XFlush(shared.display);
	}

	void X11Native::setInputFocus(bool takeIt) {
		auto &shared = SharedX11::get();
		if (impl->window == 0) return;
		XSetInputFocus(shared.display, takeIt ? impl->window : impl->parent, RevertToParent, CurrentTime);
		XFlush(shared.display);
	}

	int X11Native::connectionFd() {
		auto &shared = SharedX11::get();
		if (shared.display == nullptr && !shared.ensure()) return -1;
		return XConnectionNumber(shared.display);
	}

	bool X11Native::nextEvent(uint64_t &windowId, X11NativeEvent &event) {
		auto &shared = SharedX11::get();
		if (shared.display == nullptr) return false;

		while (XPending(shared.display) > 0) {
			XEvent ev;
			XNextEvent(shared.display, &ev);
			if (XFilterEvent(&ev, 0)) continue; // input-method internal events

			windowId	= (uint64_t) ev.xany.window;
			event		= {};
			event.shift = false;
			event.alt	= false;

			switch (ev.type) {
				case Expose:
					if (ev.xexpose.count != 0) continue;
					event.type = X11NativeEvent::Type::Exposed;
					return true;

				case FocusIn: event.type = X11NativeEvent::Type::FocusGained; return true;
				case FocusOut: event.type = X11NativeEvent::Type::FocusLost; return true;

				case MotionNotify:
					event.type = X11NativeEvent::Type::Motion;
					event.x	   = (float) ev.xmotion.x;
					event.y	   = (float) ev.xmotion.y;
					return true;

				case ButtonPress: {
					const unsigned button = ev.xbutton.button;
					event.x				  = (float) ev.xbutton.x;
					event.y				  = (float) ev.xbutton.y;
					event.shift			  = (ev.xbutton.state & ShiftMask) != 0;
					event.alt			  = (ev.xbutton.state & Mod1Mask) != 0;
					if (button >= 4 && button <= 7) {
						// Scroll wheel, one notch per press; GLFW's X11 axes.
						event.type	  = X11NativeEvent::Type::Scroll;
						event.scrollX = button == 6 ? 1.f : button == 7 ? -1.f : 0.f;
						event.scrollY = button == 4 ? 1.f : button == 5 ? -1.f : 0.f;
						return true;
					}
					event.type	 = X11NativeEvent::Type::ButtonDown;
					event.button = (int) button;
					return true;
				}

				case ButtonRelease: {
					const unsigned button = ev.xbutton.button;
					if (button >= 4 && button <= 7) continue; // scroll: press only
					event.type	 = X11NativeEvent::Type::ButtonUp;
					event.button = (int) button;
					event.x		 = (float) ev.xbutton.x;
					event.y		 = (float) ev.xbutton.y;
					return true;
				}

				case KeyPress: {
					event.type	 = X11NativeEvent::Type::KeyDown;
					event.shift	 = (ev.xkey.state & ShiftMask) != 0;
					event.alt	 = (ev.xkey.state & Mod1Mask) != 0;
					event.keysym = (unsigned long) XLookupKeysym(&ev.xkey, 0);
					// Printable input via the window's input method when there is
					// one (dead keys, compose, non-latin layouts), else latin-1.
					char buf[64] = {};
					int len		 = 0;
					auto &xics	 = xicRegistry();
					if (auto it = xics.find(ev.xany.window); it != xics.end()) {
						Status status = 0;
						len = Xutf8LookupString(it->second, &ev.xkey, buf, sizeof(buf) - 1, nullptr, &status);
						if (status != XLookupChars && status != XLookupBoth) len = 0;
					} else {
						KeySym ks = 0;
						len		  = XLookupString(&ev.xkey, buf, sizeof(buf) - 1, &ks, nullptr);
					}
					if (len > 0) {
						event.textUtf8.assign(buf, (size_t) len);
					}
					return true;
				}

				case KeyRelease:
					event.type	 = X11NativeEvent::Type::KeyUp;
					event.keysym = (unsigned long) XLookupKeysym(&ev.xkey, 0);
					return true;

				default: continue; // ConfigureNotify etc: nothing to translate
			}
		}
		return false;
	}

	float X11Native::detectPixelScale() {
		// Xft.dpi from the root window's resource string (what GTK/Qt apps use),
		// then GDK_SCALE, else 1.0. Parsed by hand to keep Xrm out of it.
		auto &shared = SharedX11::get();
		if (shared.display == nullptr && !shared.ensure()) return 1.f;
		if (const char *resources = XResourceManagerString(shared.display); resources != nullptr) {
			if (const char *entry = strstr(resources, "Xft.dpi:"); entry != nullptr) {
				const float dpi = (float) atof(entry + strlen("Xft.dpi:"));
				if (dpi > 0.f) return dpi / 96.f;
			}
		}
		if (const char *gdkScale = getenv("GDK_SCALE"); gdkScale != nullptr && *gdkScale != '\0') {
			const float scale = (float) atof(gdkScale);
			if (scale > 0.f) return scale;
		}
		return 1.f;
	}

} // namespace mzgl

#endif // __linux__ && !__ANDROID__

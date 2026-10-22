// The mzgl half of the Linux plugin editor embedding: owns the editor stack
// (Plugin/PluginEditor/Graphics/EventDispatcher) and dispatches the neutral
// events X11Native translates from Xlib. Structure mirrors what
// MzglVST3View_win.cpp's Impl does with a child HWND and D3D11; see
// X11PluginView.h for the threading/run-loop contract and X11Native.h for why
// the Xlib half is a separate translation unit.
#if defined(__linux__) && !defined(__ANDROID__)

#	include "X11PluginView.h"
#	include "X11Native.h"

#	include "Plugin.h"
#	include "PluginEditor.h"
#	include "Graphics.h"
#	include "EventDispatcher.h"
#	include "util.h"
#	include "log.h"
#	include "filesystem.h"

#	include <map>

namespace mzgl {

	namespace {

		// X11 keysym protocol constants (keysymdef.h values - stable forever;
		// spelled out here because including Xlib headers in an mzgl TU is the
		// one thing this file must never do).
		enum : unsigned long {
			kKeyBackSpace = 0xff08,
			kKeyTab		  = 0xff09,
			kKeyReturn	  = 0xff0d,
			kKeyEscape	  = 0xff1b,
			kKeyDelete	  = 0xffff,
			kKeyLeft	  = 0xff51,
			kKeyUp		  = 0xff52,
			kKeyRight	  = 0xff53,
			kKeyDown	  = 0xff54,
			kKeyPageUp	  = 0xff55,
			kKeyPageDown  = 0xff56,
			kKeyKPEnter	  = 0xff8d,
			kKeyShiftL	  = 0xffe1,
			kKeyShiftR	  = 0xffe2,
			kKeyControlL  = 0xffe3,
			kKeyControlR  = 0xffe4,
			kKeyAltL	  = 0xffe9,
			kKeyAltR	  = 0xffea,
			kKeySuperL	  = 0xffeb,
			kKeySuperR	  = 0xffec,
		};

		int keysymToMzglKey(unsigned long ks) {
			switch (ks) {
				case kKeyLeft: return MZ_KEY_LEFT;
				case kKeyRight: return MZ_KEY_RIGHT;
				case kKeyDown: return MZ_KEY_DOWN;
				case kKeyUp: return MZ_KEY_UP;
				case kKeyBackSpace:
				case kKeyDelete: return MZ_KEY_DELETE;
				case kKeyTab: return MZ_KEY_TAB;
				case kKeyEscape: return MZ_KEY_ESCAPE;
				case kKeyReturn:
				case kKeyKPEnter: return MZ_KEY_RETURN;
				case kKeyShiftL:
				case kKeyShiftR: return MZ_KEY_SHIFT;
				case kKeyControlL:
				case kKeyControlR: return MZ_KEY_CTRL;
				case kKeyAltL:
				case kKeyAltR: return MZ_KEY_ALT;
				case kKeySuperL:
				case kKeySuperR: return MZ_KEY_CMD;
				case kKeyPageUp: return MZ_KEY_INCREMENT;
				case kKeyPageDown: return MZ_KEY_DECREMENT;
				default:
					// Letters/digits: uppercase ASCII, the same convention as
					// convertGlfwKeyToMzgl and the Windows view's vkToMzglKey.
					if (ks >= 'a' && ks <= 'z') return (int) (ks - 'a' + 'A');
					if (ks < 256) return (int) ks;
					return 0;
			}
		}

	} // namespace

	struct X11PluginView::Impl {
		X11Native native;

		std::shared_ptr<Plugin> plugin;
		std::shared_ptr<PluginEditor> editor;
		std::shared_ptr<Graphics> graphics;
		std::shared_ptr<EventDispatcher> dispatcher;

		bool needsSetup {true};
		bool inFrame {false};
		bool focused {false};

		// Mouse state, as DesktopWindowEventHandler keeps it for the GLFW app.
		float mouseX {0};
		float mouseY {0};
		enum Button { Left = 0, Right = 1, Middle = 2, NumButtons };
		bool buttons[NumButtons] {};

		// window id -> view: the shared X connection's events reach the right
		// instance whichever view's handleXEvents() drains them.
		static std::map<uint64_t, Impl *> &registry() {
			static std::map<uint64_t, Impl *> views;
			return views;
		}

		bool anyButtonDown() const {
			for (bool b: buttons) {
				if (b) return true;
			}
			return false;
		}

		static int touchIdForButton(Button b) {
			switch (b) {
				case Right: return RightMouseButton;
				case Middle: return MiddleMouseButton;
				default: return 0;
			}
		}

		static Button buttonFor(int xButton) { return xButton == 3 ? Right : xButton == 2 ? Middle : Left; }

		void renderFrame() {
			if (!native.created() || !dispatcher || inFrame) return;
			if (!native.makeCurrent()) return;
			inFrame = true;

			const bool firstFrame = needsSetup;
			if (firstFrame) {
				// Same first-draw sequence as the Windows view and MZMetalView:
				// mzgl's main-thread id and timers, then the app's setup().
				initMZGL(editor);
				dispatcher->setup();
				needsSetup = false;
			}
			dispatcher->runFrame();
			native.swapBuffers();

			inFrame = false;
			if (firstFrame) {
				editor->pluginViewAppeared();
			}

			// Keyboard focus is only ours while a text field is being edited (see
			// ButtonDown); hand it back so the host's shortcuts keep working.
			if (graphics && graphics->textInputReceiver == nullptr && focused) {
				native.setInputFocus(false);
			}
		}

		void handleEvent(const X11NativeEvent &ev) {
			auto &I = *this;
			using T = X11NativeEvent::Type;
			switch (ev.type) {
				case T::Exposed: renderFrame(); break;

				case T::FocusGained: I.focused = true; break;
				case T::FocusLost: I.focused = false; break;

				case T::Motion: {
					if (!I.dispatcher) break;
					I.mouseX = ev.x;
					I.mouseY = ev.y;
					if (!I.anyButtonDown()) {
						I.dispatcher->touchOver(I.mouseX, I.mouseY);
					} else {
						for (int b = 0; b < NumButtons; b++) {
							if (I.buttons[b]) {
								I.dispatcher->touchMoved(I.mouseX, I.mouseY, touchIdForButton((Button) b));
							}
						}
					}
					break;
				}

				case T::Scroll: {
					if (!I.dispatcher) break;
					// DesktopWindowEventHandler's gains: alt = zoom, shift = swap
					// axes, SCROLL_SPEED on both.
					constexpr float scrollSpeed = 5.f;
					float dx					= ev.scrollX;
					float dy					= ev.scrollY;
					if (ev.alt) {
						I.dispatcher->mouseZoomed(ev.x, ev.y, dy * -0.03f);
						break;
					}
					dx *= scrollSpeed;
					dy *= scrollSpeed;
					if (ev.shift) std::swap(dx, dy);
					I.dispatcher->mouseScrolled(ev.x, ev.y, dx, dy);
					break;
				}

				case T::ButtonDown: {
					if (!I.dispatcher) break;
					const Button b = buttonFor(ev.button);
					I.mouseX	   = ev.x;
					I.mouseY	   = ev.y;
					I.buttons[b]   = true;
					I.dispatcher->touchDown(I.mouseX, I.mouseY, touchIdForButton(b));
					// Like the mac/win views, keyboard focus stays with the host -
					// except while a text field is active, which this click may
					// just have focused.
					if (I.graphics && I.graphics->textInputReceiver != nullptr) {
						I.native.setInputFocus(true);
					}
					break;
				}

				case T::ButtonUp: {
					if (!I.dispatcher) break;
					const Button b = buttonFor(ev.button);
					I.mouseX	   = ev.x;
					I.mouseY	   = ev.y;
					// Only report an up for a down we saw (the button may have
					// gone down outside our window).
					if (I.anyButtonDown()) {
						I.dispatcher->touchUp(I.mouseX, I.mouseY, touchIdForButton(b));
					}
					I.buttons[b] = false;
					break;
				}

				case T::KeyDown: {
					if (!I.dispatcher || !I.graphics) break;
					if (I.graphics->textInputReceiver != nullptr) {
						if (ev.keysym == kKeyBackSpace || ev.keysym == kKeyDelete) {
							I.dispatcher->textBackspace();
							break;
						}
						if (ev.keysym == kKeyReturn || ev.keysym == kKeyKPEnter) {
							I.dispatcher->textDone();
							break;
						}
						if (ev.keysym == kKeyEscape) {
							I.graphics->hideKeyboard();
							break;
						}
						if (!ev.textUtf8.empty() && (unsigned char) ev.textUtf8[0] >= 32
							&& ev.textUtf8[0] != 127) {
							I.dispatcher->textInput(ev.textUtf8);
							break;
						}
					}
					if (ev.keysym == kKeyTab && ev.shift) {
						I.dispatcher->keyDown(MZ_KEY_SHIFT_TAB);
						break;
					}
					if (const int key = keysymToMzglKey(ev.keysym); key != 0) {
						I.dispatcher->keyDown(key);
					}
					break;
				}

				case T::KeyUp: {
					if (!I.dispatcher) break;
					if (const int key = keysymToMzglKey(ev.keysym); key != 0) {
						I.dispatcher->keyUp(key);
					}
					break;
				}

				case T::NoEvent: break;
			}
		}

		void teardown() {
			if (editor) {
				editor->pluginViewDisappeared();
			}
			if (native.created()) registry().erase(native.windowId());
			// The editor's layer tree owns GPU resources: destroy it with our
			// context current so they land in the right (shared) object space.
			if (native.created()) native.makeCurrent();
			dispatcher.reset();
			editor.reset();
			graphics.reset();
			plugin.reset();
			native.destroy();
		}
	};

	X11PluginView::X11PluginView()
		: impl(std::make_unique<Impl>()) {
	}

	X11PluginView::~X11PluginView() {
		detach();
	}

	bool X11PluginView::attach(uint64_t parentWindowId, const Setup &setup) {
		if (impl->native.created()) return true;

		if (!setup.dataDir.empty()) {
			if (!fs::is_directory(setup.dataDir)) {
				Log::e() << "X11PluginView: no data folder at " << setup.dataDir;
			}
			setDataPath(setup.dataDir);
		}

		if (!impl->native.create(parentWindowId, setup.widthPx, setup.heightPx)) {
			return false;
		}
		if (impl->native.sampleCount() == 0) {
			Log::d() << "X11PluginView: no MSAA EGL config, rendering unsampled";
		}

		// Borrow the controller's plugin so editor knobs and host automation flow
		// through the same instance.
		impl->plugin = setup.getPlugin ? setup.getPlugin() : nullptr;
		if (!impl->plugin) impl->plugin = instantiatePlugin();
		impl->graphics					 = std::make_shared<Graphics>();
		impl->graphics->width			 = setup.widthPx;
		impl->graphics->height			 = setup.heightPx;
		impl->graphics->pixelScale		 = setup.pixelScale;
		impl->editor					 = instantiatePluginEditor(*impl->graphics, impl->plugin);
		impl->editor->nativeWindowHandle = (void *) (uintptr_t) impl->native.windowId();
		impl->dispatcher				 = std::make_shared<EventDispatcher>(impl->editor);

		impl->needsSetup = true;
		impl->dispatcher->resized();
		Impl::registry()[impl->native.windowId()] = impl.get();
		impl->renderFrame();
		return true;
	}

	void X11PluginView::detach() {
		if (impl) impl->teardown();
	}

	bool X11PluginView::attached() const {
		return impl->native.created();
	}

	int X11PluginView::connectionFd() const {
		return X11Native::connectionFd();
	}

	void X11PluginView::handleXEvents() {
		// One shared connection serves every view instance: route each event to
		// its window's owner (which may be another instance than this one).
		uint64_t windowId = 0;
		X11NativeEvent ev;
		while (X11Native::nextEvent(windowId, ev)) {
			auto &views = Impl::registry();
			if (auto it = views.find(windowId); it != views.end()) {
				it->second->handleEvent(ev);
			}
		}
	}

	void X11PluginView::renderFrame() {
		handleXEvents(); // events can sit in Xlib's buffer with the fd drained
		impl->renderFrame();
	}

	void X11PluginView::resize(int widthPx, int heightPx) {
		if (!impl->native.created() || widthPx <= 0 || heightPx <= 0) return;
		impl->native.resizeWindow(widthPx, heightPx);
		if (impl->graphics && !impl->inFrame) {
			impl->graphics->width  = widthPx;
			impl->graphics->height = heightPx;
			if (impl->dispatcher) impl->dispatcher->resized();
		}
	}

	void X11PluginView::setPixelScale(float scale) {
		if (scale <= 0.f || !impl->graphics) return;
		impl->graphics->pixelScale = scale;
		if (impl->dispatcher) impl->dispatcher->resized();
	}

	float X11PluginView::detectPixelScale() {
		return X11Native::detectPixelScale();
	}

} // namespace mzgl

#endif // __linux__ && !__ANDROID__

//
//  LinuxWebView.cpp
//  mzgl
//
//  WebKitGTK window - see LinuxWebView.h.
//

#include "LinuxWebView.h"
#include "App.h"
#include "log.h"
#include "util.h"

#include <gtk/gtk.h>
#include <dlfcn.h>

#include <optional>
#include <vector>

namespace {
	// Opaque WebKitGTK / JavaScriptCore types - we only ever pass pointers
	// through, so no webkit headers (or -dev package) are needed to build.
	struct WebKitUserContentManager;
	struct WebKitJavascriptResult;
	struct WebKitPolicyDecision;
	struct WebKitNavigationAction;
	struct WebKitURIRequest;
	struct JSCValue;

	// WebKitPolicyDecisionType / WebKitLoadEvent
	constexpr int policyNavigationAction = 0;
	constexpr int policyNewWindowAction	 = 1;
	constexpr int loadFinished			 = 3;

	struct WebKitApi {
		WebKitUserContentManager *(*userContentManagerNew)();
		gboolean (*registerScriptMessageHandler)(WebKitUserContentManager *, const char *);
		GtkWidget *(*webViewNewWithUserContentManager)(WebKitUserContentManager *);
		void (*loadUri)(GtkWidget *, const char *);
		const char *(*getTitle)(GtkWidget *);
		void (*runJavascript)(GtkWidget *, const char *, GCancellable *, GAsyncReadyCallback, gpointer);
		JSCValue *(*javascriptResultGetJsValue)(WebKitJavascriptResult *);
		char *(*jscValueToString)(JSCValue *);
		WebKitNavigationAction *(*navigationPolicyDecisionGetNavigationAction)(WebKitPolicyDecision *);
		WebKitURIRequest *(*navigationActionGetRequest)(WebKitNavigationAction *);
		const char *(*uriRequestGetUri)(WebKitURIRequest *);
		void (*policyDecisionIgnore)(WebKitPolicyDecision *);
	};

	template <typename Fn>
	bool resolve(void *lib, Fn &fn, const char *name) {
		fn = reinterpret_cast<Fn>(dlsym(lib, name));
		if (fn == nullptr) Log::e() << "LinuxWebView: missing symbol " << name;
		return fn != nullptr;
	}

	// nullptr if WebKitGTK isn't installed or GTK can't start. Never unloaded -
	// WebKit doesn't survive being dlclose()d.
	const WebKitApi *webkit() {
		static const std::optional<WebKitApi> api = []() -> std::optional<WebKitApi> {
			// The standalone has already run gtk_init(); a plugin may not have.
			if (gtk_init_check(nullptr, nullptr) != TRUE) {
				Log::e() << "LinuxWebView: gtk_init_check failed - web views unavailable";
				return std::nullopt;
			}
			// 4.1 is the GTK3 + libsoup3 build every current distro ships; 4.0
			// is the same api on libsoup2, for older ones.
			void *lib = nullptr;
			for (auto *name: {"libwebkit2gtk-4.1.so.0", "libwebkit2gtk-4.0.so.37"}) {
				if ((lib = dlopen(name, RTLD_NOW | RTLD_LOCAL)) != nullptr) break;
			}
			if (lib == nullptr) {
				Log::e() << "LinuxWebView: WebKitGTK not found (install webkit2gtk-4.1) - " << dlerror();
				return std::nullopt;
			}
			WebKitApi a {};
			// jsc_* comes in with webkit as a dependency, so dlsym on its handle finds it.
			bool ok = resolve(lib, a.userContentManagerNew, "webkit_user_content_manager_new")
					  && resolve(lib,
								 a.registerScriptMessageHandler,
								 "webkit_user_content_manager_register_script_message_handler")
					  && resolve(
						  lib, a.webViewNewWithUserContentManager, "webkit_web_view_new_with_user_content_manager")
					  && resolve(lib, a.loadUri, "webkit_web_view_load_uri")
					  && resolve(lib, a.getTitle, "webkit_web_view_get_title")
					  && resolve(lib, a.runJavascript, "webkit_web_view_run_javascript")
					  && resolve(lib, a.javascriptResultGetJsValue, "webkit_javascript_result_get_js_value")
					  && resolve(lib, a.jscValueToString, "jsc_value_to_string")
					  && resolve(lib,
								 a.navigationPolicyDecisionGetNavigationAction,
								 "webkit_navigation_policy_decision_get_navigation_action")
					  && resolve(lib, a.navigationActionGetRequest, "webkit_navigation_action_get_request")
					  && resolve(lib, a.uriRequestGetUri, "webkit_uri_request_get_uri")
					  && resolve(lib, a.policyDecisionIgnore, "webkit_policy_decision_ignore");
			if (!ok) return std::nullopt;
			return a;
		}();
		return api ? &*api : nullptr;
	}

	bool startsWith(const std::string &s, const std::string &prefix) {
		return s.compare(0, prefix.size(), prefix) == 0;
	}

	// Pages we keep in the view: local files, the dev server and inline content.
	bool staysInView(const std::string &uri) {
		for (auto *prefix: {"file:",
							"about:",
							"data:",
							"http://localhost",
							"http://127.0.0.1",
							"https://localhost",
							"https://127.0.0.1"}) {
			if (startsWith(uri, prefix)) return true;
		}
		return false;
	}

	// WebKit composites with its own EGL/GLX context in our process and leaves it
	// current on the main thread, so the app's next frame would draw into it
	// (and crash in the driver). Put back whatever was current before handing
	// control to GTK. libEGL / libGL are only looked up if already loaded (GLFW
	// and WebKit dlopen them), so neither is a link dependency.
	class ScopedGlContextRestore {
	public:
		ScopedGlContextRestore() {
			if (const auto &e = egl(); e.makeCurrent != nullptr) {
				eglDisplay = e.getCurrentDisplay();
				eglDraw	   = e.getCurrentSurface(eglDrawSurface);
				eglRead	   = e.getCurrentSurface(eglReadSurface);
				eglContext = e.getCurrentContext();
			}
			if (const auto &g = glx(); g.makeContextCurrent != nullptr) {
				glxDisplay = g.getCurrentDisplay();
				glxDraw	   = g.getCurrentDrawable();
				glxRead	   = g.getCurrentReadDrawable();
				glxContext = g.getCurrentContext();
			}
		}
		~ScopedGlContextRestore() {
			if (eglContext != nullptr && egl().getCurrentContext() != eglContext) {
				egl().makeCurrent(eglDisplay, eglDraw, eglRead, eglContext);
			}
			if (glxContext != nullptr && glx().getCurrentContext() != glxContext) {
				glx().makeContextCurrent(glxDisplay, glxDraw, glxRead, glxContext);
			}
		}

	private:
		static constexpr int eglDrawSurface = 0x3059; // EGL_DRAW
		static constexpr int eglReadSurface = 0x305A; // EGL_READ

		struct Egl {
			void *(*getCurrentDisplay)()							= nullptr;
			void *(*getCurrentSurface)(int)							= nullptr;
			void *(*getCurrentContext)()							= nullptr;
			unsigned (*makeCurrent)(void *, void *, void *, void *) = nullptr;
		};
		struct Glx {
			void *(*getCurrentDisplay)()											= nullptr;
			unsigned long (*getCurrentDrawable)()									= nullptr;
			unsigned long (*getCurrentReadDrawable)()								= nullptr;
			void *(*getCurrentContext)()											= nullptr;
			int (*makeContextCurrent)(void *, unsigned long, unsigned long, void *) = nullptr;
		};

		static void *loadedLib(std::initializer_list<const char *> names) {
			for (auto *name: names) {
				if (void *lib = dlopen(name, RTLD_LAZY | RTLD_NOLOAD)) return lib;
			}
			return nullptr;
		}

		// Looked up afresh until found - WebKit may load libEGL after our first call.
		static const Egl &egl() {
			static Egl e;
			if (e.makeCurrent == nullptr) {
				if (void *lib = loadedLib({"libEGL.so.1", "libEGL.so"})) {
					Egl found;
					if (resolve(lib, found.getCurrentDisplay, "eglGetCurrentDisplay")
						&& resolve(lib, found.getCurrentSurface, "eglGetCurrentSurface")
						&& resolve(lib, found.getCurrentContext, "eglGetCurrentContext")
						&& resolve(lib, found.makeCurrent, "eglMakeCurrent")) {
						e = found;
					}
				}
			}
			return e;
		}
		static const Glx &glx() {
			static Glx g;
			if (g.makeContextCurrent == nullptr) {
				if (void *lib = loadedLib({"libGLX.so.0", "libGL.so.1"})) {
					Glx found;
					if (resolve(lib, found.getCurrentDisplay, "glXGetCurrentDisplay")
						&& resolve(lib, found.getCurrentDrawable, "glXGetCurrentDrawable")
						&& resolve(lib, found.getCurrentReadDrawable, "glXGetCurrentReadDrawable")
						&& resolve(lib, found.getCurrentContext, "glXGetCurrentContext")
						&& resolve(lib, found.makeContextCurrent, "glXMakeContextCurrent")) {
						g = found;
					}
				}
			}
			return g;
		}

		void *eglDisplay = nullptr, *eglDraw = nullptr, *eglRead = nullptr, *eglContext = nullptr;
		void *glxDisplay	  = nullptr;
		unsigned long glxDraw = 0, glxRead = 0;
		void *glxContext = nullptr;
	};

	std::string toUri(const std::string &pathOrUrl) {
		if (pathOrUrl.find("://") != std::string::npos) return pathOrUrl;
		gchar *uri = g_filename_to_uri(pathOrUrl.c_str(), nullptr, nullptr);
		if (uri == nullptr) return pathOrUrl;
		std::string s = uri;
		g_free(uri);
		return s;
	}
} // namespace

class LinuxWebViewImpl : public std::enable_shared_from_this<LinuxWebViewImpl> {
public:
	LinuxWebViewImpl(App &app, LinuxWebView::Options options)
		: app(app)
		, options(std::move(options)) {}

	~LinuxWebViewImpl() { close(); }

	void open() {
		const auto *wk = webkit();
		if (wk == nullptr) {
			// No bridge back to the app this way, but the page is at least visible.
			launchUrl(options.url);
			return;
		}

		ScopedGlContextRestore restoreGl;
		auto *content = wk->userContentManagerNew();
		wk->registerScriptMessageHandler(content, "messages");
		wk->registerScriptMessageHandler(content, "closeWindow");
		g_signal_connect(content, "script-message-received::messages", G_CALLBACK(onScriptMessage), this);
		g_signal_connect(content, "script-message-received::closeWindow", G_CALLBACK(onCloseMessage), this);

		webView = wk->webViewNewWithUserContentManager(content);
		g_object_unref(content); // the view holds its own ref
		g_signal_connect(webView, "load-changed", G_CALLBACK(onLoadChanged), this);
		g_signal_connect(webView, "notify::title", G_CALLBACK(onTitleChanged), this);
		g_signal_connect(webView, "decide-policy", G_CALLBACK(onDecidePolicy), this);

		window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		gtk_window_set_default_size(GTK_WINDOW(window), options.width, options.height);
		gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
		gtk_container_add(GTK_CONTAINER(window), webView);
		g_signal_connect(window, "destroy", G_CALLBACK(onWindowDestroyed), this);

		wk->loadUri(webView, toUri(options.url).c_str());
		gtk_widget_show_all(window);
		gtk_window_present(GTK_WINDOW(window));

		pumpGtk(app, weak_from_this());
	}

	void callJs(const std::string &js) {
		if (webView == nullptr) return;
		// Scripts run before the page has finished loading would hit about:blank.
		if (!loaded) {
			pendingJs.push_back(js);
			return;
		}
		webkit()->runJavascript(webView, js.c_str(), nullptr, nullptr, nullptr);
	}

	void close() {
		if (window == nullptr) return;
		ScopedGlContextRestore restoreGl; // WebKit tears its GL down here
		gtk_widget_destroy(window); // -> onWindowDestroyed
	}

private:
	// GLFW's loop doesn't run GLib's, so service it once a frame while the
	// window is open. If we're already inside a GLib dispatch (a plugin whose
	// host runs a GTK loop and drives our frames from it) the host's loop is
	// servicing our sources, so leave it alone.
	static void pumpGtk(App &app, std::weak_ptr<LinuxWebViewImpl> weak) {
		auto self = weak.lock();
		if (!self || self->window == nullptr) return;
		if (g_main_depth() == 0) {
			ScopedGlContextRestore restoreGl;
			for (int i = 0; i < 100 && g_main_context_pending(nullptr); i++) {
				g_main_context_iteration(nullptr, FALSE);
			}
		}
		app.main.runOnNextMainLoop([&app, weak]() { pumpGtk(app, weak); });
	}

	static void
		onScriptMessage(WebKitUserContentManager *, WebKitJavascriptResult *result, LinuxWebViewImpl *self) {
		char *str		 = webkit()->jscValueToString(webkit()->javascriptResultGetJsValue(result));
		std::string data = str != nullptr ? str : "";
		g_free(str);

		if (data == "close") {
			self->close();
		} else if (self->options.jsCallback) {
			self->options.jsCallback(data);
		}
	}

	static void onCloseMessage(WebKitUserContentManager *, WebKitJavascriptResult *, LinuxWebViewImpl *self) {
		self->close();
	}

	static void onLoadChanged(GtkWidget *, int loadEvent, LinuxWebViewImpl *self) {
		if (loadEvent != loadFinished || self->loaded) return;
		self->loaded = true;
		auto pending = std::move(self->pendingJs);
		for (auto &js: pending) {
			self->callJs(js);
		}
	}

	static void onTitleChanged(GtkWidget *view, GParamSpec *, LinuxWebViewImpl *self) {
		const char *title = webkit()->getTitle(view);
		if (self->window != nullptr && title != nullptr) gtk_window_set_title(GTK_WINDOW(self->window), title);
	}

	static gboolean onDecidePolicy(GtkWidget *, WebKitPolicyDecision *decision, int type, LinuxWebViewImpl *self) {
		if (type != policyNavigationAction && type != policyNewWindowAction) return FALSE;
		const auto *wk = webkit();
		auto *request  = wk->navigationActionGetRequest(wk->navigationPolicyDecisionGetNavigationAction(decision));
		const char *uri = wk->uriRequestGetUri(request);
		if (uri == nullptr) return FALSE;

		// target=_blank etc - there's no second window to open in.
		bool toBrowser = type == policyNewWindowAction
						 || (self->options.openLinksInBrowser && self->loaded && !staysInView(uri));
		if (!toBrowser) return FALSE;

		launchUrl(uri);
		wk->policyDecisionIgnore(decision);
		return TRUE;
	}

	static void onWindowDestroyed(GtkWidget *, LinuxWebViewImpl *self) {
		self->window  = nullptr;
		self->webView = nullptr;
		self->loaded  = false;
		self->pendingJs.clear();
		if (self->options.onClosed) self->options.onClosed();
	}

	App &app;
	LinuxWebView::Options options;
	GtkWidget *window  = nullptr;
	GtkWidget *webView = nullptr;
	bool loaded		   = false;
	std::vector<std::string> pendingJs;
};

bool LinuxWebView::isAvailable() {
	return webkit() != nullptr;
}

LinuxWebView::LinuxWebView(App &app, Options options)
	: impl(std::make_shared<LinuxWebViewImpl>(app, std::move(options))) {
	impl->open();
}

LinuxWebView::~LinuxWebView() = default;

void LinuxWebView::callJs(const std::string &js) {
	impl->callJs(js);
}

void LinuxWebView::close() {
	impl->close();
}

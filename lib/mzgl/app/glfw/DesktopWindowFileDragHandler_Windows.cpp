#include "DesktopWindowFileDragHandler.h"
#include "log.h"
#include "util.h"
#include <Windows.h>
#include <ShlObj.h>
#include <cstring>
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_NATIVE_INCLUDE_NONE
#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>

namespace file_drag_handler {

	namespace {
		// Drag-out state (see runPendingNativeDrag / startNativeFileDrag at the
		// bottom): a drag queued by startNativeFileDrag() and run from the
		// owner's loop.
		std::function<std::string()> g_pendingDragRender;
		HWND g_pendingDragWindow	= nullptr;
		HWND g_lastRegisteredWindow = nullptr;
		bool g_dragOutActive		= false;
	} // namespace

	// Where the cursor is, in the window's own client coordinates.
	//
	// The POINTL OLE hands us is in the drag SOURCE's coordinate space
	// (Explorer: physical pixels), and this callback runs under the DPI
	// awareness of the thread that pumped it, not of hwnd - inside a DAW
	// that can be the host's context rather than the plugin window's. Both
	// mismatches put the drop on the wrong pad at 150% scaling. So ignore
	// the point, and read the cursor + convert under hwnd's own context,
	// the space its mouse messages arrive in. The context functions are
	// Win10 1607+, so resolve them at runtime (older: plain conversion).
	POINT clientPointUnderCursor(HWND hwnd, POINTL fallback) {
		using Ctx				 = void *; // DPI_AWARENESS_CONTEXT
		using GetWindowCtx		 = Ctx(WINAPI *)(HWND);
		using SetThreadCtx		 = Ctx(WINAPI *)(Ctx);
		static const auto user32 = GetModuleHandleW(L"user32.dll");
		static const auto getWindowCtx =
			user32 ? reinterpret_cast<GetWindowCtx>(GetProcAddress(user32, "GetWindowDpiAwarenessContext"))
				   : nullptr;
		static const auto setThreadCtx =
			user32 ? reinterpret_cast<SetThreadCtx>(GetProcAddress(user32, "SetThreadDpiAwarenessContext"))
				   : nullptr;
		Ctx previous = nullptr;
		if (getWindowCtx && setThreadCtx) previous = setThreadCtx(getWindowCtx(hwnd));
		POINT p {};
		if (!GetCursorPos(&p)) p = POINT {fallback.x, fallback.y};
		ScreenToClient(hwnd, &p);
		if (previous) setThreadCtx(previous);
		return p;
	}

	struct DropTarget : public IDropTarget {
		struct Deleter {
			auto operator()(DropTarget *ptr) const -> void { ptr->Release(); }
		};
		using Ptr = std::unique_ptr<DropTarget, Deleter>;
		DropTarget(HWND hwnd, Listener::Ptr listener)
			: hwnd_ {hwnd}
			, listener_ {std::move(listener)} {
			// S_OK or S_FALSE (already initialised on this thread, e.g. by a DAW
			// hosting us) both need the matching OleUninitialize. It fails with
			// RPC_E_CHANGED_MODE if the thread was set up as MTA.
			const HRESULT hr = OleInitialize(nullptr);
			oleInitialised_	 = SUCCEEDED(hr);
			if (!oleInitialised_) {
				Log::e() << "file_drag_handler: OleInitialize failed, hr=0x" << std::hex << (unsigned long) hr;
			}
		}
		virtual ~DropTarget() {
			if (oleInitialised_) OleUninitialize();
		}
		bool oleInitialised() const { return oleInitialised_; }
		// COM boilerplate //////////////////////////////////////////////////////////////
		STDMETHODIMP QueryInterface(REFIID riid, void **ppv) {
			if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IDropTarget)) {
				*ppv = static_cast<IDropTarget *>(this);
				AddRef();
				return S_OK;
			}
			*ppv = nullptr;
			return E_NOINTERFACE;
		}
		STDMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&refCount_); }
		STDMETHODIMP_(ULONG) Release() {
			LONG lRefCount = InterlockedDecrement(&refCount_);
			if (lRefCount == 0) {
				delete this;
			}
			return lRefCount;
		}
		// end of COM boilerplate ///////////////////////////////////////////////////////
		STDMETHODIMP DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) {
			if (g_dragOutActive) {
				// our own drag-out wandering back over us: not a drop, the icon
				// springs back (the mac's NSDraggingContextOutsideApplication rule)
				*pdwEffect = DROPEFFECT_NONE;
				return S_OK;
			}
			if (!areFilesBeingDragged(pDataObj)) {
				// No files are being dragged
				Log::d() << "file_drag_handler: DragEnter without CF_HDROP, ignoring";
				*pdwEffect = DROPEFFECT_NONE;
				return S_OK;
			}
			auto filePaths {retrieveFilepaths(pDataObj)};
			if (filePaths.empty()) {
				Log::d() << "file_drag_handler: DragEnter with CF_HDROP but no paths, ignoring";
				*pdwEffect = DROPEFFECT_NONE;
				return S_OK;
			}
			Log::d() << "file_drag_handler: DragEnter with " << filePaths.size() << " file(s)";
			effect_ = *pdwEffect;
			*pdwEffect &= DROPEFFECT_COPY;
			const POINT clientPt = clientPointUnderCursor(pt);
			listener_->onDragBegin(std::move(filePaths), clientPt.x, clientPt.y);
			return S_OK;
		}
		STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) {
			if (effect_ == DROPEFFECT_NONE) {
				// No files are being dragged
				*pdwEffect = DROPEFFECT_NONE;
				return S_OK;
			}
			*pdwEffect			 = effect_;
			const POINT clientPt = clientPointUnderCursor(pt);
			listener_->onDrag(clientPt.x, clientPt.y);
			return S_OK;
		}
		STDMETHODIMP DragLeave() {
			Log::d() << "file_drag_handler: DragLeave";
			effect_ = DROPEFFECT_NONE;
			listener_->onDragCancel();
			return S_OK;
		}
		STDMETHODIMP Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) {
			if (effect_ == DROPEFFECT_NONE) {
				// No files dropped
				*pdwEffect = DROPEFFECT_NONE;
				return S_OK;
			}
			*pdwEffect			 = effect_;
			const POINT clientPt = clientPointUnderCursor(pt);
			Log::d() << "file_drag_handler: Drop at client " << clientPt.x << "," << clientPt.y
					 << " (OLE screen pt " << pt.x << "," << pt.y << ")";
			listener_->onDrop(clientPt.x, clientPt.y);
			return S_OK;
		}

	private:
		auto clientPointUnderCursor(POINTL fallback) const -> POINT {
			return file_drag_handler::clientPointUnderCursor(hwnd_, fallback);
		}

		auto areFilesBeingDragged(IDataObject *pDataObj) -> bool {
			FORMATETC fmt = {CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
			return (pDataObj->QueryGetData(&fmt) == S_OK);
		}
		auto retrieveFilepaths(IDataObject *data) const -> std::vector<std::string> {
			FORMATETC fmt {CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
			STGMEDIUM medium;
			if (data->GetData(&fmt, &medium) != S_OK) {
				return {};
			}
			std::vector<std::string> out;
			if (medium.tymed == TYMED_HGLOBAL) {
				// This is a magic value passed in to DragQueryFile to get the file count
				static constexpr auto FILECOUNT {0xFFFFFFFF};
				const auto hDrop {static_cast<HDROP>(medium.hGlobal)};
				const auto fileCount {DragQueryFile(hDrop, FILECOUNT, nullptr, 0)};
				for (UINT i = 0; i < fileCount; ++i) {
					const auto pathLength {DragQueryFile(hDrop, i, nullptr, 0)};
					std::vector<char> buffer(pathLength + 1);
					if (DragQueryFile(hDrop, i, buffer.data(), pathLength + 1) != 0) {
						out.push_back(buffer.data());
					}
				}
				// No DragFinish() here: that is the WM_DROPFILES API and frees the
				// HGLOBAL itself. In OLE drag-and-drop the medium belongs to the data
				// object; ReleaseStgMedium below frees it (GlobalFree when the source
				// set no pUnkForRelease, as Reaper's media explorer does), so calling
				// both was a double free.
			}
			ReleaseStgMedium(&medium);
			return out;
		}
		HWND hwnd_;
		Listener::Ptr listener_;
		LONG refCount_ {1};
		DWORD effect_ {DROPEFFECT_NONE};
		bool oleInitialised_ {false};
	};

	struct Handler {
		HWND hwnd;
		DropTarget::Ptr dropTarget;
	};

	auto Deleter::operator()(Handler *ptr) const -> void {
		RevokeDragDrop(ptr->hwnd);
		delete ptr;
	}

	auto initWithNativeWindow(void *hwnd, Listener::Ptr listener) -> Ptr {
		auto out {Ptr(new Handler {}, Deleter {})};
		out->hwnd		= static_cast<HWND>(hwnd);
		out->dropTarget = DropTarget::Ptr(new DropTarget {out->hwnd, std::move(listener)}, DropTarget::Deleter {});
		if (!out->dropTarget->oleInitialised()) {
			// Deleter would RevokeDragDrop a never-registered window; harmless, but
			// be explicit that there's no handler.
			out->dropTarget.reset();
			return nullptr;
		}
		const HRESULT hr = RegisterDragDrop(out->hwnd, out->dropTarget.get());
		if (FAILED(hr)) {
			Log::e() << "file_drag_handler: RegisterDragDrop failed, hr=0x" << std::hex << (unsigned long) hr;
			out->dropTarget.reset();
			return nullptr;
		}
		Log::d() << "file_drag_handler: drop target registered";
		g_lastRegisteredWindow = out->hwnd;
		return out;
	}

	auto init(GLFWwindow *window, Listener::Ptr listener) -> Ptr {
		return initWithNativeWindow(glfwGetWin32Window(window), std::move(listener));
	}

	// ------------------------ native drag-out (IDropSource + IDataObject)

	struct DropSource : public IDropSource {
		explicit DropSource(std::function<void()> frame)
			: frame_ {std::move(frame)} {}
		STDMETHODIMP QueryInterface(REFIID riid, void **ppv) {
			if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IDropSource)) {
				*ppv = static_cast<IDropSource *>(this);
				AddRef();
				return S_OK;
			}
			*ppv = nullptr;
			return E_NOINTERFACE;
		}
		STDMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&refCount_); }
		STDMETHODIMP_(ULONG) Release() {
			const LONG n = InterlockedDecrement(&refCount_);
			if (n == 0) delete this;
			return n;
		}
		STDMETHODIMP QueryContinueDrag(BOOL escapePressed, DWORD keyState) {
			if (escapePressed) return DRAGDROP_S_CANCEL;
			if (!(keyState & MK_LBUTTON)) return DRAGDROP_S_DROP;
			// Called for every message DoDragDrop pumps: keep the window drawing
			// at about frame rate while the OS owns the mouse.
			if (frame_) {
				const ULONGLONG now = GetTickCount64();
				if (now - lastFrame_ >= 16) {
					lastFrame_ = now;
					frame_();
				}
			}
			return S_OK;
		}
		STDMETHODIMP GiveFeedback(DWORD) { return DRAGDROP_S_USEDEFAULTCURSORS; }

	private:
		std::function<void()> frame_;
		ULONGLONG lastFrame_ {0};
		LONG refCount_ {1};
	};

	// CF_HDROP for one file, rendered on first request so a drag that nobody
	// takes writes nothing (Explorer asks on DragEnter, DAWs typically only on
	// the drop). Only CF_HDROP: DAWs want a real path and reference it in
	// place, which is also why the file has to outlive the drag.
	struct FileDataObject : public IDataObject {
		explicit FileDataObject(std::function<std::string()> render)
			: render_ {std::move(render)} {}
		STDMETHODIMP QueryInterface(REFIID riid, void **ppv) {
			if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IDataObject)) {
				*ppv = static_cast<IDataObject *>(this);
				AddRef();
				return S_OK;
			}
			*ppv = nullptr;
			return E_NOINTERFACE;
		}
		STDMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&refCount_); }
		STDMETHODIMP_(ULONG) Release() {
			const LONG n = InterlockedDecrement(&refCount_);
			if (n == 0) delete this;
			return n;
		}
		STDMETHODIMP GetData(FORMATETC *fmt, STGMEDIUM *medium) {
			if (fmt == nullptr || medium == nullptr) return E_INVALIDARG;
			if (!isHdrop(*fmt)) return DV_E_FORMATETC;
			const auto &path = renderedPath();
			if (path.empty()) return E_FAIL;
			HGLOBAL h = makeHdrop(path);
			if (h == nullptr) return E_OUTOFMEMORY;
			medium->tymed		   = TYMED_HGLOBAL;
			medium->hGlobal		   = h;
			medium->pUnkForRelease = nullptr;
			return S_OK;
		}
		STDMETHODIMP GetDataHere(FORMATETC *, STGMEDIUM *) { return E_NOTIMPL; }
		STDMETHODIMP QueryGetData(FORMATETC *fmt) {
			return fmt != nullptr && isHdrop(*fmt) ? S_OK : DV_E_FORMATETC;
		}
		STDMETHODIMP GetCanonicalFormatEtc(FORMATETC *, FORMATETC *out) {
			if (out != nullptr) out->ptd = nullptr;
			return DATA_S_SAMEFORMATETC;
		}
		STDMETHODIMP SetData(FORMATETC *, STGMEDIUM *, BOOL) { return E_NOTIMPL; }
		STDMETHODIMP EnumFormatEtc(DWORD direction, IEnumFORMATETC **out) {
			if (direction != DATADIR_GET) return E_NOTIMPL;
			FORMATETC f = hdropFormat();
			return SHCreateStdEnumFmtEtc(1, &f, out);
		}
		STDMETHODIMP DAdvise(FORMATETC *, DWORD, IAdviseSink *, DWORD *) { return OLE_E_ADVISENOTSUPPORTED; }
		STDMETHODIMP DUnadvise(DWORD) { return OLE_E_ADVISENOTSUPPORTED; }
		STDMETHODIMP EnumDAdvise(IEnumSTATDATA **) { return OLE_E_ADVISENOTSUPPORTED; }

	private:
		static FORMATETC hdropFormat() {
			return FORMATETC {CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
		}
		static bool isHdrop(const FORMATETC &f) {
			return f.cfFormat == CF_HDROP && (f.tymed & TYMED_HGLOBAL) != 0 && f.dwAspect == DVASPECT_CONTENT;
		}
		const std::string &renderedPath() {
			if (!rendered_) {
				rendered_ = true;
				path_	  = render_ ? render_() : std::string {};
				if (path_.empty()) {
					Log::e() << "drag-out: the file wasn't rendered, nothing to drop";
				} else {
					Log::d() << "drag-out: rendered " << path_;
				}
			}
			return path_;
		}
		// DROPFILES header followed by the wide path list: each path
		// NUL-terminated, the list terminated by one more NUL.
		static HGLOBAL makeHdrop(const std::string &utf8Path) {
			const int n = MultiByteToWideChar(CP_UTF8, 0, utf8Path.c_str(), -1, nullptr, 0);
			if (n <= 0) return nullptr;
			std::vector<wchar_t> wide(n); // n includes the terminating NUL
			MultiByteToWideChar(CP_UTF8, 0, utf8Path.c_str(), -1, wide.data(), n);
			const SIZE_T bytes = sizeof(DROPFILES) + (wide.size() + 1) * sizeof(wchar_t);
			HGLOBAL h		   = GlobalAlloc(GHND, bytes); // zeroed: the list terminator
			if (h == nullptr) return nullptr;
			auto *df = static_cast<DROPFILES *>(GlobalLock(h));
			if (df == nullptr) {
				GlobalFree(h);
				return nullptr;
			}
			df->pFiles = sizeof(DROPFILES);
			df->fWide  = TRUE;
			std::memcpy(
				reinterpret_cast<char *>(df) + sizeof(DROPFILES), wide.data(), wide.size() * sizeof(wchar_t));
			GlobalUnlock(h);
			return h;
		}
		std::function<std::string()> render_;
		std::string path_;
		bool rendered_ {false};
		LONG refCount_ {1};
	};

	void runPendingNativeDrag(const std::function<void()> &frame) {
		if (g_dragOutActive || !g_pendingDragRender) return;
		auto render			= std::move(g_pendingDragRender);
		const HWND window	= g_pendingDragWindow;
		g_pendingDragRender = nullptr;
		g_pendingDragWindow = nullptr;

		auto *data		 = new FileDataObject(std::move(render));
		auto *source	 = new DropSource(frame);
		g_dragOutActive	 = true;
		DWORD effect	 = DROPEFFECT_NONE;
		const HRESULT hr = DoDragDrop(data, source, DROPEFFECT_COPY, &effect);
		g_dragOutActive	 = false;
		source->Release();
		data->Release();

		// OLE owned the mouse and swallowed the button-up that ended the drag:
		// feed the window one so the layer system's touch and the button state
		// are released (the mac view synthesises a touchUp the same way).
		if (window != nullptr) {
			const POINT p = clientPointUnderCursor(window, POINTL {0, 0});
			PostMessageW(window, WM_LBUTTONUP, 0, MAKELPARAM(p.x, p.y));
		}
		if (hr == DRAGDROP_S_DROP) {
			Log::d() << "drag-out: dropped, effect " << effect;
		} else if (hr == DRAGDROP_S_CANCEL) {
			Log::d() << "drag-out: cancelled";
		} else {
			Log::e() << "drag-out: DoDragDrop failed, hr=0x" << std::hex << (unsigned long) hr;
		}
	}

} // namespace file_drag_handler

bool startNativeFileDrag(std::function<std::string()> renderFile) {
	using namespace file_drag_handler;
	if (g_dragOutActive || g_pendingDragRender) return false;
	// the window the drag is happening in: it holds the mouse capture while a
	// button is down (both GLFW and the VST3 editor view do that)
	HWND window = GetCapture();
	if (window == nullptr) window = g_lastRegisteredWindow;
	if (window == nullptr) {
		Log::e() << "startNativeFileDrag: no window to drag from";
		return false;
	}
	g_pendingDragRender = std::move(renderFile);
	g_pendingDragWindow = window;
	return true;
}

bool startNativeFileDrag(const std::string &filePath) {
	return startNativeFileDrag([filePath]() { return filePath; });
}

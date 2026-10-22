#include "DesktopWindowFileDragHandler.h"
#include "log.h"
#include <Windows.h>
#include <ShlObj.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_NATIVE_INCLUDE_NONE
#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>

namespace file_drag_handler {

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
		// Where the cursor is, in the window's own client coordinates.
		//
		// The POINTL OLE hands us is in the drag SOURCE's coordinate space
		// (Explorer: physical pixels), and this callback runs under the DPI
		// awareness of the thread that pumped it, not of hwnd_ - inside a DAW
		// that can be the host's context rather than the plugin window's. Both
		// mismatches put the drop on the wrong pad at 150% scaling. So ignore
		// the point, and read the cursor + convert under hwnd_'s own context,
		// the space its mouse messages arrive in. The context functions are
		// Win10 1607+, so resolve them at runtime (older: plain conversion).
		auto clientPointUnderCursor(POINTL fallback) const -> POINT {
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
			if (getWindowCtx && setThreadCtx) previous = setThreadCtx(getWindowCtx(hwnd_));
			POINT p {};
			if (!GetCursorPos(&p)) p = POINT {fallback.x, fallback.y};
			ScreenToClient(hwnd_, &p);
			if (previous) setThreadCtx(previous);
			return p;
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
				DragFinish(hDrop);
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
		return out;
	}

	auto init(GLFWwindow *window, Listener::Ptr listener) -> Ptr {
		return initWithNativeWindow(glfwGetWin32Window(window), std::move(listener));
	}

} // namespace file_drag_handler

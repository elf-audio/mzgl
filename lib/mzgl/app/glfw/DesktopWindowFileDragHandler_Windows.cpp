#include "DesktopWindowFileDragHandler.h"
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
			OleInitialize(0);
		}
		virtual ~DropTarget() { OleUninitialize(); }
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
				*pdwEffect = DROPEFFECT_NONE;
				return S_OK;
			}
			auto filePaths {retrieveFilepaths(pDataObj)};
			if (filePaths.empty()) {
				*pdwEffect = DROPEFFECT_NONE;
				return S_OK;
			}
			effect_ = *pdwEffect;
			*pdwEffect &= DROPEFFECT_COPY;
			POINT clientPt {pt.x, pt.y};
			ScreenToClient(hwnd_, &clientPt);
			listener_->onDragBegin(std::move(filePaths), clientPt.x, clientPt.y);
			return S_OK;
		}
		STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) {
			if (effect_ == DROPEFFECT_NONE) {
				// No files are being dragged
				*pdwEffect = DROPEFFECT_NONE;
				return S_OK;
			}
			*pdwEffect = effect_;
			POINT clientPt {pt.x, pt.y};
			ScreenToClient(hwnd_, &clientPt);
			listener_->onDrag(clientPt.x, clientPt.y);
			return S_OK;
		}
		STDMETHODIMP DragLeave() {
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
			*pdwEffect = effect_;
			POINT clientPt {pt.x, pt.y};
			ScreenToClient(hwnd_, &clientPt);
			listener_->onDrop(clientPt.x, clientPt.y);
			return S_OK;
		}

	private:
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
	};

	struct Handler {
		HWND hwnd;
		DropTarget::Ptr dropTarget;
	};

	auto Deleter::operator()(Handler *ptr) const -> void {
		RevokeDragDrop(ptr->hwnd);
		delete ptr;
	}

	auto init(GLFWwindow *window, Listener::Ptr listener) -> Ptr {
		auto out {Ptr(new Handler {}, Deleter {})};
		out->hwnd		= reinterpret_cast<HWND>(glfwGetWin32Window(window));
		out->dropTarget = DropTarget::Ptr(new DropTarget {out->hwnd, std::move(listener)}, DropTarget::Deleter {});
		RegisterDragDrop(out->hwnd, out->dropTarget.get());
		return out;
	}

} // namespace file_drag_handler

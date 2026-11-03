#ifdef _WIN32

#	include "DesktopWindowFileDragHandler.h"
#	include "tests.h"

#	include <Windows.h>
#	include <ShlObj.h>
#	include <cstring>
#	include <string>
#	include <vector>

namespace {
	// A CF_HDROP block as Explorer / a DAW browser hands it over: DROPFILES
	// header, then each wide path NUL-terminated, then one more NUL.
	HGLOBAL makeWideHdrop(const std::vector<std::wstring> &paths) {
		std::wstring list;
		for (const auto &p: paths) {
			list += p;
			list.push_back(L'\0');
		}
		list.push_back(L'\0');
		const SIZE_T bytes = sizeof(DROPFILES) + list.size() * sizeof(wchar_t);
		HGLOBAL h		   = GlobalAlloc(GHND, bytes);
		auto *df		   = static_cast<DROPFILES *>(GlobalLock(h));
		df->pFiles		   = sizeof(DROPFILES);
		df->fWide		   = TRUE;
		std::memcpy(reinterpret_cast<char *>(df) + sizeof(DROPFILES), list.data(), list.size() * sizeof(wchar_t));
		GlobalUnlock(h);
		return h;
	}
} // namespace

// The tests binary has no UTF-8 activeCodePage manifest, so it runs under the
// machine's ANSI code page, like a VST3 inside FL Studio. The old
// DragQueryFileA route returned "\xFC" for "ü" here (on a 1252 machine).
TEST_CASE("Dropped file paths come out as UTF-8 whatever the ANSI code page", "[fileDrag]") {
	SECTION("a name with an umlaut") {
		HGLOBAL h  = makeWideHdrop({L"C:\\samples\\M\u00FCller kick.wav"});
		auto paths = file_drag_handler::pathsFromHdrop(h);
		GlobalFree(h);
		REQUIRE(paths.size() == 1);
		REQUIRE(paths[0] == "C:\\samples\\M\xC3\xBCller kick.wav");
	}

	SECTION("characters outside every ANSI code page, and several files") {
		// 🎹 (U+1F3B9, a surrogate pair) and 東京
		HGLOBAL h  = makeWideHdrop({L"C:\\a\\\U0001F3B9 loop.wav", L"C:\\b\\\u6771\u4EAC.wav", L"C:\\plain.wav"});
		auto paths = file_drag_handler::pathsFromHdrop(h);
		GlobalFree(h);
		REQUIRE(paths.size() == 3);
		REQUIRE(paths[0] == "C:\\a\\\xF0\x9F\x8E\xB9 loop.wav");
		REQUIRE(paths[1] == "C:\\b\\\xE6\x9D\xB1\xE4\xBA\xAC.wav");
		REQUIRE(paths[2] == "C:\\plain.wav");
	}

	SECTION("an unpaired surrogate (legal in NTFS) doesn't drop the file or throw") {
		std::wstring odd = L"C:\\x\\a";
		odd.push_back(static_cast<wchar_t>(0xD800));
		odd += L".wav";
		HGLOBAL h  = makeWideHdrop({odd});
		auto paths = file_drag_handler::pathsFromHdrop(h);
		GlobalFree(h);
		REQUIRE(paths.size() == 1);
		REQUIRE(paths[0] == "C:\\x\\a\xEF\xBF\xBD.wav"); // U+FFFD
	}
}

#endif

#include "pathUtil.h"
#include "mzgl_platform.h"

#if MZGL_WIN
fs::path u8winPath(const std::string &s);
#endif

fs::path u8path(const std::string &s) {
#if MZGL_WIN
	return u8winPath(s);
#endif
	return fs::path(s);
}

#if MZGL_WIN
fs::path u8winPath(const std::string &s) {
	if (s.empty()) {
		return fs::path();
	}
	int len = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int) s.size(), nullptr, 0);
	std::wstring ws(len, 0);
	MultiByteToWideChar(CP_UTF8, 0, s.data(), (int) s.size(), ws.data(), len);
	return fs::path(ws);
}
#endif
#ifdef _WIN32

#	include "util_windows.h"
#	include "log.h"

#	include <algorithm>
#	include <windows.h>

std::optional<int64_t> windowsGetAvailableMemory() {
	MEMORYSTATUSEX status {};
	status.dwLength = sizeof(status);
	if (GlobalMemoryStatusEx(&status)) {
		return static_cast<int64_t>(
			(std::min) ({status.ullAvailPhys, status.ullAvailPageFile, status.ullAvailVirtual}));
	}
	Log::e() << "Failed to query available memory";
	return std::nullopt;
}

#endif
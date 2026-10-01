#pragma once

#ifdef _WIN32
#	include <cstdint>
#	include <optional>

std::optional<int64_t> windowsGetAvailableMemory();
#endif

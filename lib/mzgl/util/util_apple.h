#pragma once

#ifdef __APPLE__
#	include <cstdint>
#	include <optional>

std::optional<int64_t> appleGetAvailableMemory();
#endif
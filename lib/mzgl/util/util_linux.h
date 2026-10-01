#pragma once

#ifdef __linux__
#	include <cstdint>
#	include <optional>

std::optional<int64_t> linuxGetAvailableMemory();
#endif
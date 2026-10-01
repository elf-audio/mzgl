#ifdef __linux__
#	include "util_linux.h"
#	include "log.h"

#	include <algorithm>
#	include <fstream>
#	include <string>

static int64_t readInt64FromFile(const std::string &path) {
	std::ifstream file(path);
	int64_t value = -1;
	if (file >> value) {
		return value;
	}
	return -1;
}

static int64_t readKeyedValue(const std::string &path, const std::string &key) {
	std::ifstream file(path);
	std::string name;
	int64_t value = 0;
	std::string rest;
	while (file >> name >> value) {
		if (name == key) {
			return value;
		}
		std::getline(file, rest);
	}
	return -1;
}

static std::string getCgroupV2Dir() {
	std::ifstream file("/proc/self/cgroup");
	std::string line;
	while (std::getline(file, line)) {
		if (line.rfind("0::", 0) == 0) {
			return "/sys/fs/cgroup" + line.substr(3);
		}
	}
	return "";
}

static int64_t getCgroupAvailableMemory() {
	int64_t limit		 = -1;
	int64_t usage		 = -1;
	int64_t inactiveFile = -1;

	const auto v2Dir = getCgroupV2Dir();
	if (!v2Dir.empty()) {
		limit		 = readInt64FromFile(v2Dir + "/memory.max");
		usage		 = readInt64FromFile(v2Dir + "/memory.current");
		inactiveFile = readKeyedValue(v2Dir + "/memory.stat", "inactive_file");
	}
	if (limit < 0 || usage < 0) {
		limit		 = readInt64FromFile("/sys/fs/cgroup/memory/memory.limit_in_bytes");
		usage		 = readInt64FromFile("/sys/fs/cgroup/memory/memory.usage_in_bytes");
		inactiveFile = readKeyedValue("/sys/fs/cgroup/memory/memory.stat", "total_inactive_file");
	}
	if (limit < 0 || usage < 0) {
		return -1;
	}
	const int64_t workingSet = usage - std::max<int64_t>(inactiveFile, 0);
	return std::max<int64_t>(limit - workingSet, 0);
}

std::optional<int64_t> linuxGetAvailableMemory() {
	const int64_t memAvailableKb = readKeyedValue("/proc/meminfo", "MemAvailable:");
	if (memAvailableKb < 0) {
		Log::e() << "Failed to query available memory";
		return std::nullopt;
	}
	const int64_t systemAvailable = memAvailableKb * 1024;
	const int64_t cgroupAvailable = getCgroupAvailableMemory();
	if (cgroupAvailable < 0) {
		return systemAvailable;
	}
	return std::min(systemAvailable, cgroupAvailable);
}

#endif
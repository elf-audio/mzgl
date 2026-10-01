#ifdef __APPLE__
#	include "util_apple.h"
#	include "log.h"

#	include <TargetConditionals.h>
#	include <mach/mach.h>
#	include <mach/mach_host.h>
#	include <os/proc.h>
#	include <sys/sysctl.h>

static std::optional<int64_t> getHostAvailableMemory() {
	uint64_t totalBytes = 0;
	size_t totalLength	= sizeof(totalBytes);
	if (sysctlbyname("hw.memsize", &totalBytes, &totalLength, nullptr, 0) != 0) {
		Log::e() << "Failed to query total memory";
		return std::nullopt;
	}

	static const mach_port_t host = mach_host_self();

	vm_size_t pageSize = 0;
	if (host_page_size(host, &pageSize) != KERN_SUCCESS) {
		Log::e() << "Failed to query page size";
		return std::nullopt;
	}

	vm_statistics64_data_t vm {};
	mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
	if (host_statistics64(host, HOST_VM_INFO64, reinterpret_cast<host_info64_t>(&vm), &count) != KERN_SUCCESS) {
		Log::e() << "Failed to query vm statistics";
		return std::nullopt;
	}

	const uint64_t appPages =
		vm.internal_page_count > vm.purgeable_count ? vm.internal_page_count - vm.purgeable_count : 0;
	const uint64_t usedBytes = (appPages + vm.wire_count + vm.compressor_page_count) * pageSize;
	if (usedBytes >= totalBytes) {
		return 0;
	}
	return static_cast<int64_t>(totalBytes - usedBytes);
}

std::optional<int64_t> appleGetAvailableMemory() {
#	if TARGET_OS_IOS && !TARGET_OS_SIMULATOR
	if (__builtin_available(iOS 13.0, *)) {
		return os_proc_available_memory();
	}
	return std::nullopt;
#	else
	return getHostAvailableMemory();
#	endif
}
#endif
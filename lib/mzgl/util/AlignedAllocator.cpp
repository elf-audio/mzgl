#include "AlignedAllocator.h"
#include <assert.h>
#include <memory>
#include <stdlib.h>
#ifdef _WIN32
#define posix_memalign(p, a, s) (((*(p)) = _aligned_malloc((s), (a))), *(p) ?0 :errno)
#endif


bool is_power_of_two(size_t x) {
	return !(x == 0) && !(x & (x - 1));
}


// taken from https://stackoverflow.com/questions/12942548/making-stdvector-allocate-aligned-memory
// modified to work on windows by Marek
void*
detail::allocate_aligned_memory(size_t align, size_t size)
{
    assert(align >= sizeof(void*));
    assert(is_power_of_two(align));

    if (size == 0) {
        return nullptr;
    }

    void* ptr = nullptr;
    int rc = posix_memalign(&ptr, align, size);

    if (rc != 0) {
        return nullptr;
    }

    return ptr;
}


void
detail::deallocate_aligned_memory(void *ptr) noexcept
{
	#ifdef _WIN32
	return _aligned_free(ptr);
	#else
    return free(ptr);
    #endif
}
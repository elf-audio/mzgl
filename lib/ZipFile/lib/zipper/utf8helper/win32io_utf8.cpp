#include "win32io_utf8.h"
#include <cstddef>
#include "filesystem.h"

extern "C"
{

    void* win32io_utf8_fopen(const char* filename, const char* mode) {

#if defined(_MSC_VER) && _MSC_VER >= 1400
        errno_t err;
        FILE* f;

        std::wstring unicodePath = fs::path(filename).wstring();
        const size_t size = std::strlen(mode);
        std::wstring wstr;
        if (size > 0) {
            wstr.resize(size);
            std::mbstowcs(&wstr[0], mode, size);
        }

        err = _wfopen_s(&f, unicodePath.c_str(), wstr.c_str());

        if (err == 0) {
            return f;
        }
#endif
        return NULL;
    }
}

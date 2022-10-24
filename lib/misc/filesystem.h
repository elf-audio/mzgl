// ghc filesystem has some warnings we want to suppress for a less noisy build
#include "choc/platform/choc_DisableAllWarnings.h"
#include <ghc/filesystem.hpp>
#include "choc/platform/choc_ReenableAllWarnings.h"

namespace fs = ghc::filesystem;

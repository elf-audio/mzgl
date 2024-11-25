// ghc filesystem has some warnings we want to suppress for a less noisy build
#include "DisableAllWarnings.h"
DISABLE_WARNINGS
#include <ghc/filesystem.hpp>
RESTORE_WARNINGS

namespace fs = ghc::filesystem;

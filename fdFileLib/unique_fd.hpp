#pragma once

// Backward-compat wrapper (prefer `fdFileLib/UniqueFd.hpp`)
#include "UniqueFd.hpp"

namespace FdFile {
using unique_fd = UniqueFd;
} // namespace FdFile

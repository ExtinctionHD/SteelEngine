#pragma once

#include "Utils/Logger.hpp"

#ifdef NDEBUG
#define Assert(expression) \
    if (!(expression))     \
    std::cout << "Assertion failed: " << #expression << ", file " << __FILE__ << ", line " << __LINE__ << "\n"
#else
#define Assert(expression) assert(expression)
#endif

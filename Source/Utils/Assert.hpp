#pragma once

#include "Utils/Logger.hpp"

#ifdef NDEBUG
#include <iostream>
#define Assert(expression) if (!(expression)) { std::cout << "Assertion failed: " << #expression << ", file " << __FILE__ << ", line " << __LINE__ << "\n"; }
#else
#include <cassert>
#define Assert(expression) assert(expression)
#endif

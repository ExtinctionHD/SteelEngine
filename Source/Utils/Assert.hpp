#pragma once

#ifdef NDEBUG
#define AssertD(expression) (expression)
#else
#include <cassert>
#define AssertD(expression) assert(expression)
#endif
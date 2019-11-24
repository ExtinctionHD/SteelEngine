#pragma once

#include <iostream>
#define LogE std::cout << "[ERROR]\t"

#ifdef NDEBUG

class FakeLog
{
public:
    FakeLog& operator<<(const std::string&)
    {
        return *this;
    }
};

static FakeLog fakeLog;

#define LogI fakeLog

#else

#define LogI std::cout << "[INFO]\t"

#endif

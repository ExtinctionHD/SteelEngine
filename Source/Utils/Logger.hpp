#pragma once
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

#define LogE fakeLog
#define LogI fakeLog

#else

#include <iostream>
#define LogE std::cout << "[ERROR]\t"
#define LogI std::cout << "[INFO]\t"

#endif


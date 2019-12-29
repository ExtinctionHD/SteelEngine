#pragma once

#include <iostream>

#define LogE std::cout << "[ERROR]\t"
#define LogI std::cout << "[INFO]\t"

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

#define LogD fakeLog

#else

#define LogD std::cout << "[INFO]\t"

#endif

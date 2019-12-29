#pragma once

#include <iostream>

#define LogE std::cout << "[ERROR] "
#define LogI std::cout << "[INFO] "

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

#define LogD LogI

#endif

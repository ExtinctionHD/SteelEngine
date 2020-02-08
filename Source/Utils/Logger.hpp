#pragma once

#define LogE std::cout << "[ERROR] "
#define LogW std::cout << "[WARNING] "
#define LogI std::cout << "[INFO] "

#ifdef NDEBUG

class FakeLog
{
public:
    FakeLog &operator<<(const std::string &)
    {
        return *this;
    }
};

static FakeLog fakeLog;

#define LogD fakeLog

#else

#define LogD LogI

#endif

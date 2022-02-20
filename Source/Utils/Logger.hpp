#pragma once

#include <string>

#define LogE std::cout << "[ERROR] "
#define LogW std::cout << "[WARNING] "
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

#define LogT std::cout << "[TIME] " << std::fixed << std::setprecision(2)

class ProgressLogger
{
public:
    ProgressLogger(const std::string& aName, float aDeltaSeconds);

    void Log(size_t current, size_t total);

    void Log(float percents);

    void End() const;

private:
    std::string name;

    float deltaSeconds = 0.0f;
    float timePointSeconds = 0.0f;
};

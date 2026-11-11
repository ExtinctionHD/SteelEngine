#pragma once

#include <chrono>
#include <iostream>
#include <sstream>
#include <string>

#define LOG_ERROR std::cout << "[ERROR] "
#define LOG_WARNING std::cout << "[WARNING] "
#define LOG_INFO std::cout << "[INFO] "

#ifdef NDEBUG

#define LOG_DEBUG if constexpr (true) {} else LOG_INFO

#else

#define LOG_DEBUG LOG_INFO

#endif

#define LOG_CONCAT_IMPL(a, b) a##b
#define LOG_CONCAT(a, b) LOG_CONCAT_IMPL(a, b)
#define LOG_SCOPE_TIMING ScopeTimingLogger LOG_CONCAT(scopeTimingLogger, __COUNTER__) = ScopeTimingMessage(__FUNCTION__)

class ScopeTimingMessage
{
public:
    ScopeTimingMessage(const char* aFunctionName);

    template <class T>
    ScopeTimingMessage& operator<<(const T& value)
    {
        message << value;
        return *this;
    }

    std::string GetText() const;

private:
    const char* functionName;
    std::ostringstream message;
};

class ScopeTimingLogger
{
public:
    ScopeTimingLogger(const ScopeTimingMessage& aMessage);
    ~ScopeTimingLogger();

    ScopeTimingLogger(const ScopeTimingLogger&) = delete;
    ScopeTimingLogger& operator=(const ScopeTimingLogger&) = delete;

private:
    std::string message;
    std::chrono::steady_clock::time_point startTime;
};

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

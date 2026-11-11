#include <iostream>
#include <iomanip>

#include "Utils/Logger.hpp"

#include "Utils/TimeHelpers.hpp"

namespace Details
{
    static thread_local size_t scopeDepth = 0;

    constexpr size_t kLiteralsSize = std::string_view("[PROGRESS] : 100.0 %").size();
}

ScopeTimingMessage::ScopeTimingMessage(const char* aFunctionName)
    : functionName(aFunctionName)
{
}

std::string ScopeTimingMessage::GetText() const
{
    const std::string text = message.str();
    return text.empty() ? functionName : text;
}

ScopeTimingLogger::ScopeTimingLogger(const ScopeTimingMessage& aMessage)
    : message(aMessage.GetText())
    , startTime(std::chrono::steady_clock::now())
{
    LOG_INFO << std::string(Details::scopeDepth * 2, ' ') << message << " started\n";

    ++Details::scopeDepth;
}

ScopeTimingLogger::~ScopeTimingLogger()
{
    const double milliseconds = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - startTime).count();

    --Details::scopeDepth;

    std::ostringstream duration;
    duration << std::fixed << std::setprecision(1) << milliseconds;

    LOG_INFO << std::string(Details::scopeDepth * 2, ' ') << message << ": " << duration.str() << " ms\n";
}

ProgressLogger::ProgressLogger(const std::string& aName, float aDeltaSeconds)
    : name(aName)
    , deltaSeconds(aDeltaSeconds)
{
    timePointSeconds = Timer::GetGlobalSeconds();
}

void ProgressLogger::Log(size_t current, size_t total)
{
    Log(static_cast<float>(current * 100) / static_cast<float>(total));
}

void ProgressLogger::Log(float percents)
{
    const float currentTimePointSeconds = Timer::GetGlobalSeconds();
    if (currentTimePointSeconds - timePointSeconds > deltaSeconds)
    {
        timePointSeconds = currentTimePointSeconds;
        std::cout << "\r[PROGRESS] " << name << ": " << std::fixed << std::setprecision(1) << percents << " %";
    }
}

void ProgressLogger::End() const
{
    const std::string spaces(name.size() + Details::kLiteralsSize, ' ');
    std::cout << "\r" << spaces << "\r";
}

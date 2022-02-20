#include <iostream>

#include "Utils/Logger.hpp"

#include "Utils/TimeHelpers.hpp"

namespace Details
{
    constexpr size_t kLiteralsSize = std::string_view("[PROGRESS] : 100.0 %").size();
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
        std::cout << "\r[PROGRESS] " << name << ": " << std::setprecision(1) << percents << " %";
    }
}

void ProgressLogger::End() const
{
    const std::string spaces(name.size() + Details::kLiteralsSize, ' ');
    std::cout << "\r" << spaces << "\r";
}

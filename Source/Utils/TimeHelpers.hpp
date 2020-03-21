#pragma once

#include <chrono>

using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

class Timer
{
public:
    float GetDeltaSeconds();

private:
    std::optional<TimePoint> lastTimePoint;
};

namespace Time
{
    float GetGlobalSeconds();
}

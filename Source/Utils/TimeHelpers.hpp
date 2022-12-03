#pragma once

#include <chrono>

using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

class Timer
{
public:
    static float GetGlobalSeconds();

    float GetDeltaSeconds();

private:
    std::optional<TimePoint> lastTimePoint;
};

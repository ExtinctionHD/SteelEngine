#pragma once

#include <chrono>

using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

class Timer
{
public:
    static float GetGlobalSeconds();

    float GetDeltaSeconds() const;

    void Tick();

private:
    float lastDeltaSeconds = 0.0f;

    std::optional<TimePoint> lastTimePoint;
};

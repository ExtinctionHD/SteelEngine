#include "Utils/Timer.hpp"

#include "Utils/Helpers.hpp"

using namespace std::chrono;

namespace STime
{
    static const TimePoint startTimePoint = high_resolution_clock::now();
}

float Timer::GetGlobalSeconds()
{
    const TimePoint now = high_resolution_clock::now();
    return duration_cast<nanoseconds>(now - STime::startTimePoint).count() * Numbers::kNano;
}

float Timer::GetDeltaSeconds()
{
    float deltaSeconds = 0.0f;

    const TimePoint now = high_resolution_clock::now();

    if (lastTimePoint.has_value())
    {
        deltaSeconds = duration_cast<nanoseconds>(now - lastTimePoint.value()).count() * Numbers::kNano;
    }

    lastTimePoint = now;

    return deltaSeconds;
}


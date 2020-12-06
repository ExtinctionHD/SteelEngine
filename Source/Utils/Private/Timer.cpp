#include "Utils/Timer.hpp"

#include "Utils/Helpers.hpp"

using namespace std::chrono;

namespace Details
{
    static const TimePoint startTimePoint = high_resolution_clock::now();
}

float Timer::GetGlobalSeconds()
{
    const TimePoint now = high_resolution_clock::now();
    const uint64_t nanosecondCount = duration_cast<nanoseconds>(now - Details::startTimePoint).count();
    return static_cast<float>(nanosecondCount) * Numbers::kNano;
}

float Timer::GetDeltaSeconds()
{
    float deltaSeconds = 0.0f;

    const TimePoint now = high_resolution_clock::now();

    if (lastTimePoint.has_value())
    {
        const uint64_t nanosecondCount = duration_cast<nanoseconds>(now - lastTimePoint.value()).count();
        deltaSeconds = static_cast<float>(nanosecondCount)* Numbers::kNano;
    }

    lastTimePoint = now;

    return deltaSeconds;
}

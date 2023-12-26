#include "Utils/TimeHelpers.hpp"

#include "Utils/Helpers.hpp"

using namespace std::chrono;

namespace Details
{
    static const TimePoint globalStartTimePoint = high_resolution_clock::now();

    static uint64_t GetNanosecondCount(const TimePoint& start, const TimePoint& end)
    {
        return duration_cast<nanoseconds>(end - start).count();
    }

    static float GetDeltaSeconds(const TimePoint& start, const TimePoint& end)
    {
        const uint64_t nanosecondCount = GetNanosecondCount(start, end);

        return static_cast<float>(nanosecondCount) * Metric::kNano;
    }
}

float Timer::GetGlobalSeconds()
{
    const TimePoint now = high_resolution_clock::now();
    const uint64_t nanosecondCount = duration_cast<nanoseconds>(now - Details::globalStartTimePoint).count();
    return static_cast<float>(nanosecondCount) * Metric::kNano;
}

float Timer::GetDeltaSeconds() const
{
    return lastDeltaSeconds;
}

void Timer::Tick()
{
    const TimePoint now = high_resolution_clock::now();

    if (lastTimePoint.has_value())
    {
        lastDeltaSeconds = Details::GetDeltaSeconds(lastTimePoint.value(), now);
    }

    lastTimePoint = now;
}

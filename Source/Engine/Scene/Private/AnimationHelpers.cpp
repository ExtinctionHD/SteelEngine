#include "Engine/Scene/AnimationHelpers.hpp"

#include "Utils/Assert.hpp"
#include "Utils/Helpers.hpp"

namespace AnimationHelpers
{
    constexpr size_t BeastNumber = 666666;

    static size_t FindCurIndex(float time, const std::vector<float>& times)
    {
        size_t index = BeastNumber; // fast handling of absense of first frame info (t == 0)
        for (size_t i = 0; i < times.size(); ++i)
        {
            if (time > times[i])
            {
                index = i;
            }
            else
            {
                break;
            }
        }
        return index;
    }

    static TrackUid curTrackUid = 0;
    static AnimationUid curAnimUid = 0;

    TrackUid GenerateTrackUid()
    {
        TrackUid result = curTrackUid;
        curTrackUid += 1;

        return result;
    }

    AnimationUid GenerateAnimationUid()
    {
        AnimationUid result = curAnimUid;
        curAnimUid += 1;

        return result;
    }

    eKeyFrameInterpolation ParseInterpolationType(const std::string& str)
    {
        if (str == "LINEAR")
        {
            return eKeyFrameInterpolation::Linear;
        }
        else if (str == "STEP")
        {
            return eKeyFrameInterpolation::Step;
        }
        else if (str == "CUBICSPLINE")
        {
            return eKeyFrameInterpolation::CubicSpline;
        }

        Assert(false);
        return eKeyFrameInterpolation::Linear;
    }

    glm::vec4 FindValueAt(const KeyFrameAnimationTrack& track, float time, bool isLooped)
    {
        size_t maxIndex = track.keyFrameTimes.size() - 1;
        if (isLooped)
        {
            time = std::fmod(time, track.keyFrameTimes[maxIndex]);
        }

        size_t index = AnimationHelpers::FindCurIndex(time, track.keyFrameTimes);

        if (track.interpolation == eKeyFrameInterpolation::Step || index == maxIndex
            || index == AnimationHelpers::BeastNumber)
        {
            if (index == AnimationHelpers::BeastNumber)
            {
                index = 0;
            }
            return track.values.at(index);
        }

        const float tDiff = track.keyFrameTimes[index + 1] - track.keyFrameTimes[index];
        float term = (time - track.keyFrameTimes[index]) / tDiff;

        const glm::vec4& v1 = track.values.at(index);
        const glm::vec4& v2 = track.values.at(index + 1);

        return glm::mix(v1, v2, term);
    }

    glm::quat FindQuatValueAt(const KeyFrameAnimationTrack& track, float time, bool isLooped)
    {
        size_t maxIndex = track.keyFrameTimes.size() - 1;
        if (isLooped)
        {
            time = std::fmod(time, track.keyFrameTimes[maxIndex]);
        }

        size_t index = AnimationHelpers::FindCurIndex(time, track.keyFrameTimes);

        if (track.interpolation == eKeyFrameInterpolation::Step || index == maxIndex
            || index == AnimationHelpers::BeastNumber)
        {
            if (index == AnimationHelpers::BeastNumber)
            {
                index = 0;
            }

            const glm::vec4& val = track.values.at(index);
            return glm::quat(val.w, val.x, val.y, val.z);
        }

        const float tDiff = track.keyFrameTimes[index + 1] - track.keyFrameTimes[index];
        float term = (time - track.keyFrameTimes[index]) / tDiff;

        const glm::vec4& v1 = track.values.at(index);
        const glm::vec4& v2 = track.values.at(index + 1);

        const glm::quat q1 = glm::quat(v1.w, v1.x, v1.y, v1.z);
        const glm::quat q2 = glm::quat(v2.w, v2.x, v2.y, v2.z);

        return glm::slerp(q1, q2, term);
    }

    bool IsTrackFinished(const KeyFrameAnimationTrack& track, float time)
    {
        return time > track.keyFrameTimes.back();
    }
}

#include "Engine/Scene/Systems/AnimationSystem.hpp"

#include "Engine/Scene/Components/AnimationComponent.hpp"
#include "Engine/Scene/Scene.hpp"
#include "Engine/Scene/Components/Components.hpp"

namespace Details
{
    template <class T>
    static T GetValue(const glm::vec4& value)
    {
        static_assert(
            std::is_same_v<T, glm::quat> ||
            std::is_same_v<T, glm::vec3>);

        if constexpr (std::is_same_v<T, glm::quat>)
        {
            return glm::quat(value.w, value.x, value.y, value.z);
        }
        else
        {
            return glm::vec3(value.x, value.y, value.z);
        }
    }

    template <class T>
    static T LerpValue(const glm::vec4& a, const glm::vec4 b, float t)
    {
        static_assert(
            std::is_same_v<T, glm::quat> ||
            std::is_same_v<T, glm::vec3>);

        if constexpr (std::is_same_v<T, glm::quat>)
        {
            return glm::slerp(GetValue<T>(a), GetValue<T>(b), t);
        }
        else
        {
            return glm::mix(GetValue<T>(a), GetValue<T>(b), t);
        }
    }

    template <class T>
    static T GetValueAtTimeStamp(const AnimationTrack& track, float timeStamp)
    {
        Assert(!track.keyFrames.empty());

        const float minTimestamp = track.keyFrames.front().timeStamp;
        const float maxTimestamp = track.keyFrames.back().timeStamp;

        timeStamp = std::clamp(timeStamp, minTimestamp, maxTimestamp);

        for (size_t index = 0; index < track.keyFrames.size() - 1; ++index)
        {
            const size_t nextIndex = index + 1;

            const AnimationKeyFrame& frameA = track.keyFrames[index];
            const AnimationKeyFrame& frameB = track.keyFrames[nextIndex];

            if (timeStamp <= frameB.timeStamp)
            {
                Assert(timeStamp >= frameA.timeStamp);

                if (track.interpolation == AnimationInterpolation::eStep)
                {
                    return GetValue<T>(frameA.value);
                }

                Assert(track.interpolation == AnimationInterpolation::eLinear);

                const float timestampA = frameA.timeStamp;
                const float timestampB = frameB.timeStamp;

                const float t = Math::GetRangePercentage(timestampA, timestampB, timeStamp);

                return LerpValue<T>(frameA.value, frameB.value, t);
            }
        }

        return GetValue<T>(track.keyFrames.back().value);
    }
}

void AnimationSystem::Process(Scene& scene, float deltaSeconds)
{
    if (auto* ac = scene.ctx().find<AnimationComponent>())
    {
        for (auto& animation : ac->animations)
        {
            if (animation.state.active)
            {
                ProcessAnimation(animation, scene, deltaSeconds);
            }
        }
    }

    scene.view<AnimationComponent>().each([&](auto& ac)
        {
            for (auto& animation : ac.animations)
            {
                if (animation.state.active)
                {
                    ProcessAnimation(animation, scene, deltaSeconds);
                }
            }
        });
}

void AnimationSystem::ProcessAnimation(Animation& animation, Scene& scene, float deltaSeconds) const
{
    animation.state.time += deltaSeconds * animation.state.speed;

    if (animation.state.time > animation.duration)
    {
        if (animation.state.looped)
        {
            animation.state.time = std::fmod(animation.state.time, animation.duration);
        }
        else
        {
            animation.state.active = false;
            animation.state.time = animation.duration;
        }
    }

    float timeStamp = animation.state.time;

    if (animation.state.reverse)
    {
        timeStamp = animation.duration - animation.state.time;
    }

    for (const auto& track : animation.tracks)
    {
        auto& tc = scene.get<TransformComponent>(track.target);

        switch (track.property)
        {
        case AnimatedProperty::eTranslation:
            tc.SetLocalTranslation(Details::GetValueAtTimeStamp<glm::vec3>(track, timeStamp));
            break;
        case AnimatedProperty::eRotation:
            tc.SetLocalRotation(Details::GetValueAtTimeStamp<glm::quat>(track, timeStamp));
            break;
        case AnimatedProperty::eScale:
            tc.SetLocalScale(Details::GetValueAtTimeStamp<glm::vec3>(track, timeStamp));
            break;
        default:
            Assert(false);
            break;
        }
    }
}

#pragma once

#include <glm/vec4.hpp>
#include <map>
#include <set>
#include <string>
#include <vector>

enum class eKeyFrameInterpolation
{
    Linear,
    Step,
    CubicSpline
};

using AnimationUid = uint32_t;
using TrackUid = uint32_t;

struct KeyFrameAnimationTrack
{
    TrackUid uid;
    std::string propName;

    eKeyFrameInterpolation interpolation;
    std::vector<float> keyFrameTimes;
    std::vector<glm::vec4> values;
};

struct AnimationComponent
{
    // tracks data is in AnimationStorageComponent
    std::map<AnimationUid, std::vector<TrackUid>> animationTracks;
};

struct Animation
{
    enum class eState
    {
        Stopped,
        Playing,
        Paused,
    };

    void Start();
    void StartLooped();
    void Stop();
    void Pause();
    void Unpause();

    bool IsPlaying() const;
    bool IsPaused() const;

    AnimationUid uid;
    std::string name;
    std::set<entt::entity> animatedEntities;

    eState state = eState::Stopped;
    bool isLooped = false;
    float playbackSpeed = 1.0f;
    float curTime = 0.0f;
};

struct AnimationControlComponent
{
    std::vector<Animation> animations;
};

struct AnimationStorageComponent
{
    std::vector<KeyFrameAnimationTrack> animationTracks;
};

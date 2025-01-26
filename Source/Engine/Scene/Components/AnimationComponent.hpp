#pragma once

enum class AnimatedProperty
{
    eTranslation,
    eRotation,
    eScale
};

enum class AnimationInterpolation
{
    eStep,
    eLinear
};

struct AnimationKeyFrame
{
    float timeStamp;
    glm::vec4 value;
};

struct AnimationTrack
{
    entt::entity target;
    AnimatedProperty property;
    AnimationInterpolation interpolation;
    std::vector<AnimationKeyFrame> keyFrames;
};

struct Animation
{
    std::string name;
    std::vector<AnimationTrack> tracks;

    float time = 0.0f;
    float duration = 0.0f;
    float speed = 1.0f;

    uint32_t update : 1 = false;
    uint32_t active : 1 = false;
    uint32_t looped : 1 = false;
    uint32_t reverse : 1 = false;

    void Play();
    void Pause();
    void Reset();
};

// TODO consider AnimationStorageComponent name
struct AnimationComponent
{
    std::vector<Animation> animations;
};

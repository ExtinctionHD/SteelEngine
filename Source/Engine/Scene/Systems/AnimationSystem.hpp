#pragma once

#include "Engine/Scene/Systems/System.hpp"

class Scene;
class TransformComponent;
struct Animation;
struct AnimationStorageComponent;
struct KeyFrameAnimationTrack;

class AnimationSystem
    : public System
{
public:
	AnimationSystem();
    ~AnimationSystem() override;

    void Process(Scene& scene, float deltaSeconds) override;

private:
    void UpdateKeyFrameAnimations(Scene& scene, float deltaSeconds);

    void UpdateAnimation(Animation& anim, Scene& scene, const AnimationStorageComponent& animationStorage, float deltaSeconds);
    void UpdateAnimationTrack(const KeyFrameAnimationTrack& track, TransformComponent& tc, float animCurTime, bool isLooped);

    const float keyFrameAnimationUpdateStepSeconds = 0.0166666f;

	float timeSinceKeyFrameAnimationsUpdate = 0.0f;
};

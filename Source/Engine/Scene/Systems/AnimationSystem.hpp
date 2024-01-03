#pragma once

#include "Engine/Scene/Systems/System.hpp"

class Scene;
struct Animation;

class AnimationSystem
    : public System
{
public:
    void Process(Scene& scene, float deltaSeconds) override;

private:
    void ProcessAnimation(Animation& animation, Scene& scene, float deltaSeconds) const;
};

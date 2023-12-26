#pragma once

#include "Engine/Scene/Systems/System.hpp"

class Scene;
struct Animation2;

class AnimationSystem
    : public System
{
public:
    void Process(Scene& scene, float deltaSeconds) override;

private:
    void ProcessAnimation(Animation2& animation, Scene& scene, float deltaSeconds) const;
};

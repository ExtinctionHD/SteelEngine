#pragma once

#include "Engine/UI/ImGuiRenderer.hpp"

class Scene;

class AnimationPlayer : public ImGuiWidget
{
public:
    AnimationPlayer();

protected:
    void UpdateInternal(const Scene* scene, float deltaSeconds) const override;
};

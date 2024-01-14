#pragma once

#include "Engine/UI/ImGuiWidget.hpp"

class Scene;

class AnimationPlayer : public ImGuiWidget
{
public:
    AnimationPlayer();

protected:
    void BuildInternal(const Scene* scene, float deltaSeconds) override;
};

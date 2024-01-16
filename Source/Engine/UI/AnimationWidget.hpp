#pragma once

#include "Engine/UI/ImGuiWidget.hpp"

class Scene;

class AnimationWidget : public ImGuiWidget
{
public:
    AnimationWidget();

protected:
    void BuildInternal(Scene* scene, float deltaSeconds) override;
};

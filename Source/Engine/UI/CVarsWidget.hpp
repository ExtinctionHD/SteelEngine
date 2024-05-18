#pragma once

#include "Engine/UI/ImGuiWidget.hpp"

class Scene;

class CVarsWidget : public ImGuiWidget
{
public:
    CVarsWidget();

protected:
    void BuildInternal(Scene* scene, float deltaSeconds) override;
};

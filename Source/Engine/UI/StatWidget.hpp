#pragma once

#include "Engine/UI/ImGuiWidget.hpp"

class Scene;

class StatWidget : public ImGuiWidget
{
public:
    StatWidget();

protected:
    void BuildInternal(Scene* scene, float deltaSeconds) override;
};

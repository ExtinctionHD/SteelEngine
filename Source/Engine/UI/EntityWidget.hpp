#pragma once

#include "Engine/UI/ImGuiWidget.hpp"

class Scene;

class EntityWidget : public ImGuiWidget
{
public:
    EntityWidget();

protected:
    void BuildInternal(Scene* scene, float deltaSeconds) override;
};

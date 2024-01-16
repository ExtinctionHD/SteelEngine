#pragma once

#include "Engine/UI/ImGuiWidget.hpp"

class Scene;

class HierarchyWidget : public ImGuiWidget
{
public:
    HierarchyWidget();

protected:
    void BuildInternal(Scene* scene, float deltaSeconds) override;
};

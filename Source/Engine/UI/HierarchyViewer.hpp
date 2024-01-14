#pragma once

#include "Engine/UI/ImGuiWidget.hpp"

class Scene;

class HierarchyViewer : public ImGuiWidget
{
public:
    HierarchyViewer();

protected:
    void BuildInternal(Scene* scene, float deltaSeconds) override;
};

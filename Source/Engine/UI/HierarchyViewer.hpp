#pragma once

#include "Engine/UI/ImGuiRenderer.hpp"

class Scene;

class HierarchyViewer : public ImGuiWidget
{
public:
    HierarchyViewer();

protected:
    void UpdateInternal(const Scene* scene, float deltaSeconds) override;
};

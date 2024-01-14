#pragma once

#include "Engine/UI/ImGuiRenderer.hpp"

class Scene;

class StatViewer : public ImGuiWidget
{
public:
    StatViewer();

protected:
    void UpdateInternal(const Scene* scene, float deltaSeconds) override;
};

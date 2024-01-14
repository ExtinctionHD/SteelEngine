#pragma once

#include "Engine/UI/ImGuiWidget.hpp"

class Scene;

class StatViewer : public ImGuiWidget
{
public:
    StatViewer();

protected:
    void BuildInternal(const Scene* scene, float deltaSeconds) override;
};

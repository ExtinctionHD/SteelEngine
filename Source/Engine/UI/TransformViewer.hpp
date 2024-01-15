#pragma once

#include "Engine/UI/ImGuiWidget.hpp"

class Scene;

// TODO rework to EntityViewer
class TransformViewer : public ImGuiWidget
{
public:
    TransformViewer();

protected:
    void BuildInternal(Scene* scene, float deltaSeconds) override;
};

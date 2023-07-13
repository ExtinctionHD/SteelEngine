#pragma once

#include "Engine/Scene/Components/CameraComponent.hpp"
#include "Engine/Render/PathTracingRenderer.hpp"

class ProbeRenderer
        : PathTracingRenderer
{
public:
    ProbeRenderer(const Scene* scene_);

    Texture CaptureProbe(const glm::vec3& position);

private:
    CameraComponent cameraComponent;
};

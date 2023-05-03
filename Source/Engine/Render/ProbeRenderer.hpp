#pragma once

#include "Engine/Camera.hpp"
#include "Engine/Render/PathTracingRenderer.hpp"
#include "Vulkan/Resources/ImageHelpers.hpp"

class ProbeRenderer
        : PathTracingRenderer
{
public:
    ProbeRenderer(const Scene* scene_);

    Texture CaptureProbe(const glm::vec3& position);

private:
    CameraComponent cameraComponent;
};

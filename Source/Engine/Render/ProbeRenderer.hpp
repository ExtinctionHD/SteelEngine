#pragma once

#include "Engine/Scene/Components/CameraComponent.hpp"
#include "Engine/Render/PathTracingRenderer.hpp"
#include "Vulkan/Resources/ImageHelpers.hpp"

class ProbeRenderer
        : PathTracingRenderer
{
public:
    ProbeRenderer(const Scene* scene_);

    BaseImage CaptureProbe(const glm::vec3& position);

private:
    CameraComponent cameraComponent;
};

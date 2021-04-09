#pragma once

#include "Engine/Camera.hpp"
#include "Engine/Render/PathTracer.hpp"
#include "Vulkan/Resources/ImageHelpers.hpp"

class ProbeRenderer
        : private Camera
        , private PathTracer
{
public:
    ProbeRenderer(ScenePT* scene_, Environment* environment_);

    Texture CaptureProbe(const glm::vec3& position);

private:
    void SetupRenderTargetsDescriptorSet(const ImageHelpers::CubeFacesViews& probeFacesViews);
};

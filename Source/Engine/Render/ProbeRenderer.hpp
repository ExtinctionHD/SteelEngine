#pragma once

#include "Engine/Render/SceneRenderer.hpp"
#include "Engine/Render/Stages/PathTracingStage.hpp"
#include "Engine/Render/Vulkan/Resources/ImageHelpers.hpp"

class ProbeRenderer
        : PathTracingStage
{
public:
    ProbeRenderer(const Scene* scene_);

    BaseImage CaptureProbe(const glm::vec3& position) const;

private:
    SceneRenderContext dummyContext;
};

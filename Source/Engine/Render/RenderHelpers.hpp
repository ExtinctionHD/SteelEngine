#pragma once

#include "Engine/Scene/Material.hpp"
#include "Engine/Scene/Scene.hpp"

class RenderPass;
class PipelineCache;
class GraphicsPipeline;
class DescriptorProvider;

using MaterialPipelinePred = std::function<bool(MaterialFlags)>;

namespace RenderHelpers
{
    vk::Rect2D GetSwapchainRenderArea();

    vk::Viewport GetSwapchainViewport();

    void PushEnvironmentDescriptorData(const Scene& scene, DescriptorProvider& descriptorProvider);
    void PushLightVolumeDescriptorData(const Scene& scene, DescriptorProvider& descriptorProvider);
    void PushRayTracingDescriptorData(const Scene& scene, DescriptorProvider& descriptorProvider);

    std::set<MaterialFlags> CachePipelines(const Scene& scene, PipelineCache& cache, const MaterialPipelinePred& pred);
}

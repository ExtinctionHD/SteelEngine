#pragma once

#include "Engine/Scene/Material.hpp"
#include "Engine/Scene/Scene.hpp"

class RenderPass;
class GraphicsPipeline;
class DescriptorProvider;
class MaterialPipelineCache;

using MaterialPipelinePred = std::function<bool(MaterialFlags)>;

namespace RenderHelpers
{
    void PushEnvironmentDescriptorData(const Scene& scene, DescriptorProvider& descriptorProvider);
    void PushLightVolumeDescriptorData(const Scene& scene, DescriptorProvider& descriptorProvider);
    void PushRayTracingDescriptorData(const Scene& scene, DescriptorProvider& descriptorProvider);

    // TODO move to base class - MaterialRenderStage
    std::set<MaterialFlags> CacheMaterialPipelines(const Scene& scene,
            MaterialPipelineCache& cache, const MaterialPipelinePred& pred);
}

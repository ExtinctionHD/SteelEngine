#pragma once

#include "Engine/Scene/Material.hpp"
#include "Engine/Scene/Scene.hpp"

class RenderPass;
class GraphicsPipeline;
class DescriptorProvider;
class MaterialPipelineCache;
struct TopLevelAS;

using MaterialPipelinePred = std::function<bool(MaterialFlags)>;

namespace RenderHelpers
{
    void PushEnvironmentDescriptorData(
            DescriptorProvider& descriptorProvider, const Scene& scene);

    void PushLightVolumeDescriptorData(
            DescriptorProvider& descriptorProvider, const Scene& scene);

    void PushRayTracingDescriptorData(
            DescriptorProvider& descriptorProvider, const Scene& scene, const TopLevelAS& tlas);

    // TODO move to base class - MaterialRenderStage
    std::set<MaterialFlags> CacheMaterialPipelines(const Scene& scene,
            MaterialPipelineCache& cache, const MaterialPipelinePred& pred);
}

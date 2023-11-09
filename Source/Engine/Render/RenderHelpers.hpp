#pragma once

#include "Engine/Scene/Material.hpp"
#include "Engine/Scene/Scene.hpp"

class RenderPass;
class GraphicsPipeline;

struct MaterialPipeline
{
    MaterialFlags materialFlags;
    std::unique_ptr<GraphicsPipeline> pipeline;
};

using CreateMaterialPipelinePred = std::function<bool(MaterialFlags)>;

using MaterialPipelineCreator =
        std::function<std::unique_ptr<GraphicsPipeline>(const RenderPass&, const Scene&, MaterialFlags)>;

namespace RenderHelpers
{
    vk::Rect2D GetSwapchainRenderArea();

    vk::Viewport GetSwapchainViewport();

    void PushEnvironmentDescriptorData(const Scene& scene, DescriptorProvider& descriptorProvider);
    void PushLightVolumeDescriptorData(const Scene& scene, DescriptorProvider& descriptorProvider);
    void PushRayTracingDescriptorData(const Scene& scene, DescriptorProvider& descriptorProvider);

    std::vector<MaterialPipeline> CreateMaterialPipelines(const Scene& scene, const RenderPass& renderPass,
            const CreateMaterialPipelinePred& createPipelinePred, const MaterialPipelineCreator& pipelineCreator);

    void UpdateMaterialPipelines(std::vector<MaterialPipeline>& pipelines, const Scene& scene,
            const RenderPass& renderPass, const CreateMaterialPipelinePred& createPipelinePred,
            const MaterialPipelineCreator& pipelineCreator);

    bool CheckPipelinesCompatibility(const std::vector<MaterialPipeline>& pipelines);
}

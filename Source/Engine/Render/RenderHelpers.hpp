#pragma once

#include "Engine/Render/Vulkan/Pipelines/GraphicsPipeline.hpp"

#include "Engine/Scene/Material.hpp"
#include "Engine/Scene/Scene.hpp"

class RenderPass;

struct CameraData
{
    std::vector<vk::Buffer> buffers;
};

struct MaterialPipeline
{
    MaterialFlags materialFlags;
    std::unique_ptr<GraphicsPipeline> pipeline;
};

using CreateMaterialPipelinePred = std::function<bool(MaterialFlags)>;

using MaterialPipelineCreator = std::function<std::unique_ptr<GraphicsPipeline>(
        const RenderPass&, const MaterialFlags&, const Scene&)>;

namespace RenderHelpers
{
    CameraData CreateCameraData(uint32_t bufferCount, vk::DeviceSize bufferSize);

    vk::Rect2D GetSwapchainRenderArea();

    vk::Viewport GetSwapchainViewport();

    void PushEnvironmentDescriptorData(const Scene& scene, DescriptorProvider& descriptorProvider);
    void PushLightVolumeDescriptorData(const Scene& scene, DescriptorProvider& descriptorProvider);
    void PushRayTracingDescriptorData(const Scene& scene, DescriptorProvider& descriptorProvider);

    std::vector<MaterialPipeline> CreateMaterialPipelines(
            const Scene& scene, const RenderPass& renderPass,
            const CreateMaterialPipelinePred& createPipelinePred,
            const MaterialPipelineCreator& pipelineCreator);

    bool CheckPipelinesCompatibility(const std::vector<MaterialPipeline>& pipelines);
}

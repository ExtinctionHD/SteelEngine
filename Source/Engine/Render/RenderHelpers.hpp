#pragma once

#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"

#include "Engine/Scene/Material.hpp"

class RenderPass;

struct CameraData
{
    std::vector<vk::Buffer> buffers;
    MultiDescriptorSet descriptorSet;
};

struct MaterialPipeline
{
    MaterialFlags materialFlags;
    std::unique_ptr<GraphicsPipeline> pipeline;
};

using CreateMaterialPipelinePred = std::function<bool(MaterialFlags)>;

using MaterialPipelineCreator = std::function<std::unique_ptr<GraphicsPipeline>(const RenderPass&,
        const std::vector<vk::DescriptorSetLayout>&, const MaterialFlags&, const Scene&)>;

namespace RenderHelpers
{
    CameraData CreateCameraData(uint32_t bufferCount,
            vk::DeviceSize bufferSize, vk::ShaderStageFlags shaderStages);

    vk::Rect2D GetSwapchainRenderArea();

    vk::Viewport GetSwapchainViewport();

    DescriptorSet CreateMaterialDescriptorSet(
            const Scene& scene, vk::ShaderStageFlags stageFlags);

    DescriptorSet CreateLightingDescriptorSet(
            const Scene& scene, vk::ShaderStageFlags stageFlags);

    DescriptorSet CreateRayTracingDescriptorSet(
            const Scene& scene, vk::ShaderStageFlags stageFlags, 
            bool includeMaterialBuffer);

    std::vector<MaterialPipeline> CreateMaterialPipelines(
            const Scene& scene, const RenderPass& renderPass,
            const std::vector<vk::DescriptorSetLayout>& layouts,
            const CreateMaterialPipelinePred& createPipelinePred,
            const MaterialPipelineCreator& pipelineCreator);
}

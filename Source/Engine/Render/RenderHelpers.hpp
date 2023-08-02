#pragma once

#include "Engine/Render/FrameLoop.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Pipelines/GraphicsPipeline.hpp"

#include "Engine/Scene/Material.hpp"
#include "Engine/Scene/Scene.hpp"

class RenderPass;

struct MaterialPipeline
{
    MaterialFlags materialFlags;
    std::unique_ptr<GraphicsPipeline> pipeline;
};

using CreateMaterialPipelinePred = std::function<bool(MaterialFlags)>;

using MaterialPipelineCreator = std::function<
    std::unique_ptr<GraphicsPipeline>(const RenderPass&, const Scene&, MaterialFlags)>;

namespace RenderHelpers
{
    vk::Rect2D GetSwapchainRenderArea();

    vk::Viewport GetSwapchainViewport();

    void PushEnvironmentDescriptorData(const Scene& scene, DescriptorProvider& descriptorProvider);
    void PushLightVolumeDescriptorData(const Scene& scene, DescriptorProvider& descriptorProvider);
    void PushRayTracingDescriptorData(const Scene& scene, DescriptorProvider& descriptorProvider);

    std::vector<MaterialPipeline> CreateMaterialPipelines(
            const Scene& scene, const RenderPass& renderPass,
            const CreateMaterialPipelinePred& createPipelinePred,
            const MaterialPipelineCreator& pipelineCreator);

    void AppendMaterialPipelines(std::vector<MaterialPipeline>& pipelines,
            const Scene& scene, const RenderPass& renderPass,
            const CreateMaterialPipelinePred& createPipelinePred,
            const MaterialPipelineCreator& pipelineCreator);

    bool CheckPipelinesCompatibility(const std::vector<MaterialPipeline>& pipelines);

    // TODO move to ResourceHelpers and use everywhere
    template <class T>
    void DestroyResource(T resource)
    {
        static_assert(std::is_same_v<T, Texture>
            || std::is_same_v<T, vk::Sampler>
            || std::is_same_v<T, vk::Image>
            || std::is_same_v<T, vk::Buffer>
            || std::is_same_v<T, vk::AccelerationStructureKHR>);

        if constexpr (std::is_same_v<T, Texture>)
        {
            VulkanContext::textureManager->DestroyTexture(resource);
        }
        if constexpr (std::is_same_v<T, vk::Sampler>)
        {
            VulkanContext::textureManager->DestroySampler(resource);
        }
        if constexpr (std::is_same_v<T, vk::Image>)
        {
            VulkanContext::imageManager->DestroyImage(resource);
        }
        if constexpr (std::is_same_v<T, vk::Buffer>)
        {
            VulkanContext::bufferManager->DestroyBuffer(resource);
        }
        if constexpr (std::is_same_v<T, vk::AccelerationStructureKHR>)
        {
            VulkanContext::accelerationStructureManager->DestroyAccelerationStructure(resource);
        }
    }

    template <class T>
    void DestroyResourceDelayed(T resource)
    {
        RenderContext::frameLoop->DestroyResource([resource]()
            {
                DestroyResource(resource);
            });
    }
}

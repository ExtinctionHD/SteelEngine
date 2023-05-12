#include "Engine/Render/Stages/LightingStage.hpp"

#include "Engine/Render/RenderContext.hpp"
#include "Engine/Render/Stages/GBufferStage.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Pipelines/PipelineHelpers.hpp"
#include "Engine/Render/Vulkan/Pipelines/ComputePipeline.hpp"
#include "Engine/Render/Vulkan/Resources/BufferHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/ImageHelpers.hpp"
#include "Engine/Scene/GlobalIllumination.hpp"
#include "Engine/Scene/StorageComponents.hpp"
#include "Engine/Scene/Components.hpp"
#include "Engine/Scene/Scene.hpp"

namespace Details
{
    static constexpr glm::uvec3 kWorkGroupSize(8, 8, 1);

    static CameraData CreateCameraData()
    {
        const uint32_t bufferCount = VulkanContext::swapchain->GetImageCount();

        constexpr vk::DeviceSize bufferSize = sizeof(glm::mat4);

        return RenderHelpers::CreateCameraData(bufferCount, bufferSize);
    }

    static std::unique_ptr<ComputePipeline> CreatePipeline(const Scene& scene)
    {
        const auto& materialComponent = scene.ctx().get<MaterialStorageComponent>();

        const bool lightVolumeEnabled = scene.ctx().contains<LightVolumeComponent>();

        const ShaderDefines defines{
            std::make_pair("LIGHT_COUNT", static_cast<uint32_t>(scene.view<LightComponent>().size())),
            std::make_pair("MATERIAL_COUNT", static_cast<uint32_t>(materialComponent.materials.size())),
            std::make_pair("RAY_TRACING_ENABLED", static_cast<uint32_t>(Config::kRayTracingEnabled)),
            std::make_pair("LIGHT_VOLUME_ENABLED", static_cast<uint32_t>(lightVolumeEnabled)),
        };

        const ShaderModule shaderModule = VulkanContext::shaderManager->CreateComputeShaderModule(
                Filepath("~/Shaders/Hybrid/Lighting.comp"), kWorkGroupSize, defines);

        std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(shaderModule);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

        return pipeline;
    }

    static void AppendGBufferDescriptorData(const std::vector<vk::ImageView>& imageViews, DescriptorSetData& data)
    {
        for (size_t i = 0; i < imageViews.size(); ++i)
        {
            if (ImageHelpers::IsDepthFormat(GBufferStage::kFormats[i]))
            {
                data.push_back(DescriptorHelpers::GetData(RenderContext::texelSampler, imageViews[i]));
            }
            else
            {
                data.push_back(DescriptorHelpers::GetStorageData(imageViews[i]));
            }
        }
    }

    static void UpdateDescriptors(const FrameDescriptorProvider& descriptorProvider, const Scene& scene,
            const std::vector<vk::ImageView>& gBufferImageViews, const CameraData& cameraData)
    {
        const auto& renderComponent = scene.ctx().get<RenderStorageComponent>();
        const auto& textureComponent = scene.ctx().get<TextureStorageComponent>();

        DescriptorSetData globalDescriptorSetData{
            DescriptorHelpers::GetData(renderComponent.lightBuffer)
        };

        Details::AppendGBufferDescriptorData(gBufferImageViews, globalDescriptorSetData);

        RenderHelpers::AppendEnvironmentDescriptorData(scene, globalDescriptorSetData);
        RenderHelpers::AppendLightVolumeDescriptorData(scene, globalDescriptorSetData);
        RenderHelpers::AppendRayTracingDescriptorData(scene, globalDescriptorSetData);

        if constexpr (Config::kRayTracingEnabled)
        {
            globalDescriptorSetData.push_back(DescriptorHelpers::GetData(renderComponent.materialBuffer));
            globalDescriptorSetData.push_back(DescriptorHelpers::GetData(textureComponent.textures));
        }

        descriptorProvider.UpdateGlobalDescriptorSet(globalDescriptorSetData);

        for (uint32_t i = 0; i < descriptorProvider.GetSliceCount(); ++i)
        {
            const DescriptorSetData frameDescriptorSetData{
                DescriptorHelpers::GetData(cameraData.buffers[i]),
                DescriptorHelpers::GetStorageData(VulkanContext::swapchain->GetImageViews()[i])
            };

            descriptorProvider.UpdateFrameDescriptorSet(i, frameDescriptorSetData);
        }
    }
}

LightingStage::LightingStage(const std::vector<vk::ImageView>& gBufferImageViews_)
    : cameraData(Details::CreateCameraData())
    , gBufferImageViews(gBufferImageViews_)
{}

LightingStage::~LightingStage()
{
    RemoveScene();

    for (const auto& buffer : cameraData.buffers)
    {
        VulkanContext::bufferManager->DestroyBuffer(buffer);
    }
}

void LightingStage::RegisterScene(const Scene* scene_)
{
    RemoveScene();

    scene = scene_;

    pipeline = Details::CreatePipeline(*scene);

    descriptorProvider = std::make_unique<FrameDescriptorProvider>(pipeline->GetDescriptorSetLayouts());

    Details::UpdateDescriptors(*descriptorProvider, *scene, gBufferImageViews, cameraData);
}

void LightingStage::RemoveScene()
{
    if (!scene)
    {
        return;
    }

    descriptorProvider.reset();

    pipeline.reset();

    scene = nullptr;
}

void LightingStage::Execute(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const auto& cameraComponent = scene->ctx().get<CameraComponent>();

    const glm::mat4& view = cameraComponent.viewMatrix;
    const glm::mat4& proj = cameraComponent.projMatrix;

    const glm::mat4 inverseProjView = glm::inverse(view) * glm::inverse(proj);

    BufferHelpers::UpdateBuffer(commandBuffer, cameraData.buffers[imageIndex],
            GetByteView(inverseProjView), SyncScope::kWaitForNone, SyncScope::kComputeShaderRead);

    const vk::Image swapchainImage = VulkanContext::swapchain->GetImages()[imageIndex];
    const vk::Extent2D& extent = VulkanContext::swapchain->GetExtent();
    const glm::vec3& cameraPosition = cameraComponent.location.position;

    const ImageLayoutTransition layoutTransition{
        vk::ImageLayout::ePresentSrcKHR,
        vk::ImageLayout::eGeneral,
        PipelineBarrier{
            SyncScope::kWaitForNone,
            SyncScope::kComputeShaderWrite
        }
    };

    ImageHelpers::TransitImageLayout(commandBuffer, swapchainImage,
            ImageHelpers::kFlatColor, layoutTransition);

    pipeline->Bind(commandBuffer);

    pipeline->BindDescriptorSets(commandBuffer, 0, descriptorProvider->GetDescriptorSlice(imageIndex));

    pipeline->PushConstant(commandBuffer, "cameraPosition", cameraPosition);

    const glm::uvec3 groupCount = PipelineHelpers::CalculateWorkGroupCount(extent, Details::kWorkGroupSize);

    commandBuffer.dispatch(groupCount.x, groupCount.y, groupCount.z);
}

void LightingStage::Resize(const std::vector<vk::ImageView>& gBufferImageViews_)
{
    gBufferImageViews = gBufferImageViews_;

    pipeline = Details::CreatePipeline(*scene);

    descriptorProvider = std::make_unique<FrameDescriptorProvider>(pipeline->GetDescriptorSetLayouts());

    Details::UpdateDescriptors(*descriptorProvider, *scene, gBufferImageViews, cameraData);
}

void LightingStage::ReloadShaders()
{
    pipeline = Details::CreatePipeline(*scene);

    descriptorProvider = std::make_unique<FrameDescriptorProvider>(pipeline->GetDescriptorSetLayouts());

    Details::UpdateDescriptors(*descriptorProvider, *scene, gBufferImageViews, cameraData);
}

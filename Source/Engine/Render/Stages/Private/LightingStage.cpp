#include "Engine/Render/Stages/LightingStage.hpp"

#include "Engine/Render/RenderContext.hpp"
#include "Engine/Render/SceneRenderer.hpp"
#include "Engine/Render/Stages/GBufferStage.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Pipelines/PipelineHelpers.hpp"
#include "Engine/Render/Vulkan/Pipelines/ComputePipeline.hpp"
#include "Engine/Render/Vulkan/Resources/ImageHelpers.hpp"
#include "Engine/Scene/GlobalIllumination.hpp"
#include "Engine/Scene/Scene.hpp"
#include "Engine/Components/StorageComponents.hpp"
#include "Engine/Components/Components.hpp"

namespace Details
{
    static constexpr glm::uvec3 kWorkGroupSize(8, 8, 1);

    static std::unique_ptr<ComputePipeline> CreatePipeline(const Scene& scene)
    {
        const bool lightVolumeEnabled = scene.ctx().contains<LightVolumeComponent>();

        const ShaderDefines defines{
            std::make_pair("LIGHT_COUNT", static_cast<uint32_t>(scene.view<LightComponent>().size())),
            std::make_pair("RAY_TRACING_ENABLED", static_cast<uint32_t>(Config::kRayTracingEnabled)),
            std::make_pair("LIGHT_VOLUME_ENABLED", static_cast<uint32_t>(lightVolumeEnabled)),
        };

        const ShaderModule shaderModule = VulkanContext::shaderManager->CreateComputeShaderModule(
                Filepath("~/Shaders/Hybrid/Lighting.comp"), kWorkGroupSize, defines);

        std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(shaderModule);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

        return pipeline;
    }

    static void UpdateDescriptors(DescriptorProvider& descriptorProvider, const Scene& scene,
            const std::vector<vk::ImageView>& gBufferImageViews)
    {
        const auto& renderComponent = scene.ctx().get<RenderSceneComponent>();
        const auto& textureComponent = scene.ctx().get<TextureStorageComponent>();

        const TextureSampler depthTexture{ gBufferImageViews.back(), RenderContext::texelSampler };

        if (renderComponent.lightBuffer)
        {
            descriptorProvider.PushGlobalData("lights", renderComponent.lightBuffer);
        }

        Assert(GBufferStage::kColorAttachmentCount == 4);
        descriptorProvider.PushGlobalData("gBufferTexture0", gBufferImageViews[0]);
        descriptorProvider.PushGlobalData("gBufferTexture1", gBufferImageViews[1]);
        descriptorProvider.PushGlobalData("gBufferTexture2", gBufferImageViews[2]);
        descriptorProvider.PushGlobalData("gBufferTexture3", gBufferImageViews[3]);
        descriptorProvider.PushGlobalData("depthTexture", depthTexture);

        RenderHelpers::PushEnvironmentDescriptorData(scene, descriptorProvider);
        RenderHelpers::PushLightVolumeDescriptorData(scene, descriptorProvider);
        RenderHelpers::PushRayTracingDescriptorData(scene, descriptorProvider);

        if constexpr (Config::kRayTracingEnabled)
        {
            descriptorProvider.PushGlobalData("materials", renderComponent.materialBuffer);
            descriptorProvider.PushGlobalData("materialTextures", &textureComponent.textureSamplers);
        }

        for (uint32_t i = 0; i < VulkanContext::swapchain->GetImageCount(); ++i)
        {
            descriptorProvider.PushSliceData("frame", renderComponent.frameBuffers[i]);
            descriptorProvider.PushSliceData("renderTarget", VulkanContext::swapchain->GetImageViews()[i]);
        }

        descriptorProvider.FlushData();
    }
}

LightingStage::LightingStage(const std::vector<vk::ImageView>& gBufferImageViews_)
    : gBufferImageViews(gBufferImageViews_)
{}

LightingStage::~LightingStage()
{
    RemoveScene();
}

void LightingStage::RegisterScene(const Scene* scene_)
{
    RemoveScene();

    scene = scene_;

    pipeline = Details::CreatePipeline(*scene);

    descriptorProvider = pipeline->CreateDescriptorProvider();

    Details::UpdateDescriptors(*descriptorProvider, *scene, gBufferImageViews);
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
    const vk::Image swapchainImage = VulkanContext::swapchain->GetImages()[imageIndex];
    const vk::Extent2D& extent = VulkanContext::swapchain->GetExtent();

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

    pipeline->BindDescriptorSets(commandBuffer, descriptorProvider->GetDescriptorSlice(imageIndex));

    const glm::uvec3 groupCount = PipelineHelpers::CalculateWorkGroupCount(extent, Details::kWorkGroupSize);

    commandBuffer.dispatch(groupCount.x, groupCount.y, groupCount.z);
}

void LightingStage::Resize(const std::vector<vk::ImageView>& gBufferImageViews_)
{
    gBufferImageViews = gBufferImageViews_;

    pipeline = Details::CreatePipeline(*scene);

    descriptorProvider = pipeline->CreateDescriptorProvider();

    Details::UpdateDescriptors(*descriptorProvider, *scene, gBufferImageViews);
}

void LightingStage::ReloadShaders()
{
    pipeline = Details::CreatePipeline(*scene);

    descriptorProvider = pipeline->CreateDescriptorProvider();

    Details::UpdateDescriptors(*descriptorProvider, *scene, gBufferImageViews);
}

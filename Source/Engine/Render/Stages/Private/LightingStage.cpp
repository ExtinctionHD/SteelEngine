#include "Engine/Render/Stages/LightingStage.hpp"

#include "Engine/Render/Stages/GBufferStage.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Pipelines/PipelineHelpers.hpp"
#include "Engine/Render/Vulkan/Pipelines/ComputePipeline.hpp"
#include "Engine/Render/Vulkan/Resources/ImageHelpers.hpp"
#include "Engine/Scene/Scene.hpp"
#include "Engine/Scene/Components/Components.hpp"

namespace Details
{
    static constexpr glm::uvec3 kWorkGroupSize(8, 8, 1);

    static std::unique_ptr<ComputePipeline> CreatePipeline()
    {
        const ShaderDefines shaderDefines{
            { "RAY_TRACING_ENABLED", Config::kRayTracingEnabled },
            { "LIGHT_VOLUME_ENABLED", Config::kGlobalIlluminationEnabled },
        };

        const ShaderModule shaderModule = VulkanContext::shaderManager->CreateComputeShaderModule(
                Filepath("~/Shaders/Hybrid/Lighting.comp"), kWorkGroupSize, shaderDefines);

        std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(shaderModule);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

        return pipeline;
    }

    static void CreateDescriptors(DescriptorProvider& descriptorProvider, const Scene& scene,
            const std::vector<RenderTarget>& gBufferTargets)
    {
        const auto& renderComponent = scene.ctx().get<RenderContextComponent>();
        const auto& textureComponent = scene.ctx().get<TextureStorageComponent>();

        const vk::Sampler texelSampler = TextureCache::GetSampler(DefaultSampler::eTexelClamp);

        const Texture depthTexture{ gBufferTargets.back(), texelSampler };

        descriptorProvider.PushGlobalData("lights", renderComponent.lightBuffer);

        static_assert(GBufferStage::kColorAttachmentCount == 4);
        descriptorProvider.PushGlobalData("gBufferTexture0", gBufferTargets[0].view);
        descriptorProvider.PushGlobalData("gBufferTexture1", gBufferTargets[1].view);
        descriptorProvider.PushGlobalData("gBufferTexture2", gBufferTargets[2].view);
        descriptorProvider.PushGlobalData("gBufferTexture3", gBufferTargets[3].view);
        descriptorProvider.PushGlobalData("depthTexture", &depthTexture);

        RenderHelpers::PushEnvironmentDescriptorData(scene, descriptorProvider);
        RenderHelpers::PushLightVolumeDescriptorData(scene, descriptorProvider);

        if (scene.ctx().contains<RayTracingContextComponent>())
        {
            RenderHelpers::PushRayTracingDescriptorData(scene, descriptorProvider);

            descriptorProvider.PushGlobalData("materials", renderComponent.materialBuffer);
            descriptorProvider.PushGlobalData("materialTextures", &textureComponent.textures);
        }

        for (uint32_t i = 0; i < VulkanContext::swapchain->GetImageCount(); ++i)
        {
            descriptorProvider.PushSliceData("frame", renderComponent.frameBuffers[i]);
            descriptorProvider.PushSliceData("renderTarget", VulkanContext::swapchain->GetImageViews()[i]);
        }

        descriptorProvider.FlushData();
    }
}

LightingStage::LightingStage(const std::vector<RenderTarget>& gBufferTargets_)
    : gBufferTargets(gBufferTargets_)
{
    pipeline = Details::CreatePipeline();

    descriptorProvider = pipeline->CreateDescriptorProvider();
}

LightingStage::~LightingStage()
{
    RemoveScene();
}

void LightingStage::RegisterScene(const Scene* scene_)
{
    RemoveScene();

    scene = scene_;
    Assert(scene);

    Details::CreateDescriptors(*descriptorProvider, *scene, gBufferTargets);
}

void LightingStage::RemoveScene()
{
    if (!scene)
    {
        return;
    }

    descriptorProvider->Clear();

    scene = nullptr;
}

void LightingStage::Update() const
{
    Assert(scene);

    if (const auto* rayTracingComponent = scene->ctx().find<RayTracingContextComponent>())
    {
        const auto& textureComponent = scene->ctx().get<TextureStorageComponent>();
        const auto& geometryComponent = scene->ctx().get<GeometryStorageComponent>();

        if (geometryComponent.updated || rayTracingComponent->updated)
        {
            RenderHelpers::PushRayTracingDescriptorData(*scene, *descriptorProvider);
        }

        if (textureComponent.updated)
        {
            descriptorProvider->PushGlobalData("materialTextures", &textureComponent.textures);
        }

        descriptorProvider->FlushData();
    }
}

void LightingStage::Execute(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const vk::Image swapchainImage = VulkanContext::swapchain->GetImages()[imageIndex];
    const vk::Extent2D& extent = VulkanContext::swapchain->GetExtent();

    const uint32_t lightCount = static_cast<uint32_t>(scene->view<LightComponent>().size());

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

    pipeline->PushConstant(commandBuffer, "lightCount", lightCount);

    const glm::uvec3 groupCount = PipelineHelpers::CalculateWorkGroupCount(extent, Details::kWorkGroupSize);

    commandBuffer.dispatch(groupCount.x, groupCount.y, groupCount.z);
}

void LightingStage::Resize(const std::vector<RenderTarget>& gBufferTargets_)
{
    gBufferTargets = gBufferTargets_;

    if (scene)
    {
        Details::CreateDescriptors(*descriptorProvider, *scene, gBufferTargets);
    }
}

void LightingStage::ReloadShaders()
{
    Assert(scene);

    pipeline = Details::CreatePipeline();

    descriptorProvider = pipeline->CreateDescriptorProvider();

    Details::CreateDescriptors(*descriptorProvider, *scene, gBufferTargets);
}

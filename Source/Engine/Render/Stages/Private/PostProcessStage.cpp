#include "Engine/Render/Stages/PostProcessStage.hpp"

#include "Engine/Render/SceneRenderer.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Pipelines/ComputePipeline.hpp"
#include "Engine/Render/Vulkan/Pipelines/PipelineHelpers.hpp"

namespace Details
{
    static std::unique_ptr<ComputePipeline> CreatePipeline()
    {
        const ShaderModule shaderModule = VulkanContext::shaderManager->CreateComputeShaderModule(
                Filepath("~/Shaders/Compute/PostProcess.comp"));

        std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(shaderModule);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

        return pipeline;
    }

    static void CreateDescriptors(DescriptorProvider& descriptorProvider, const SceneRenderContext& context)
    {
        descriptorProvider.PushGlobalData("sceneColorTexture", context.gBuffer.sceneColor.view);

        for (const auto& frameBuffer : context.uniforms.frames)
        {
            descriptorProvider.PushSliceData("frame", frameBuffer);
        }

        for (const auto& view : VulkanContext::swapchain->GetImageViews())
        {
            descriptorProvider.PushSliceData("renderTarget", view);
        }

        descriptorProvider.FlushData();
    }
}

PostProcessStage::PostProcessStage(const SceneRenderContext& context_)
    : RenderStage(context_)
{
    pipeline = Details::CreatePipeline();

    descriptorProvider = pipeline->CreateDescriptorProvider();
}

PostProcessStage::~PostProcessStage()
{
    PostProcessStage::RemoveScene();
}

void PostProcessStage::RegisterScene(const Scene* scene_)
{
    RenderStage::RegisterScene(scene_);

    Details::CreateDescriptors(*descriptorProvider, context);
}

void PostProcessStage::RemoveScene()
{
    if (!scene)
    {
        return;
    }

    descriptorProvider->Clear();

    scene = nullptr;
}

void PostProcessStage::Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
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

    ImageHelpers::TransitImageLayout(commandBuffer, swapchainImage, ImageHelpers::kFlatColor, layoutTransition);

    pipeline->Bind(commandBuffer);

    pipeline->BindDescriptorSets(commandBuffer, descriptorProvider->GetDescriptorSlice(imageIndex));

    const glm::uvec3 groupCount = PipelineHelpers::CalculateWorkGroupCount(extent);

    commandBuffer.dispatch(groupCount.x, groupCount.y, groupCount.z);
}

void PostProcessStage::Resize()
{
    if (scene)
    {
        Details::CreateDescriptors(*descriptorProvider, context);
    }
}

void PostProcessStage::ReloadShaders()
{
    pipeline = Details::CreatePipeline();

    descriptorProvider = pipeline->CreateDescriptorProvider();

    if (scene)
    {
        Details::CreateDescriptors(*descriptorProvider, context);
    }
}

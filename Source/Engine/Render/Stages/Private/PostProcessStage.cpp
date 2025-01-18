#include "Engine/Render/Stages/PostProcessStage.hpp"

#include "Engine/Render/SceneRenderer.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Pipelines/ComputePipeline.hpp"
#include "Engine/Render/Vulkan/Pipelines/PipelineHelpers.hpp"

namespace Details
{
    static constexpr glm::uvec3 kWorkGroupSize(8, 8, 1);

    static std::unique_ptr<ComputePipeline> CreatePipeline()
    {
        const ShaderModule shaderModule = VulkanContext::shaderManager->CreateComputeShaderModule(
                Filepath("~/Shaders/Compute/PostProcess.comp"), kWorkGroupSize);

        std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(shaderModule);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

        return pipeline;
    }

    static void CreateDescriptors(DescriptorProvider& descriptorProvider,
            const Scene& scene, const GBufferAttachments& gBuffer)
    {
        const auto& renderComponent = scene.ctx().get<RenderContextComponent>();

        descriptorProvider.PushGlobalData("sceneColorTexture", gBuffer.sceneColor.view);

        for (uint32_t i = 0; i < VulkanContext::swapchain->GetImageCount(); ++i)
        {
            descriptorProvider.PushSliceData("frame", renderComponent.uniforms.frames[i]);
            descriptorProvider.PushSliceData("renderTarget", VulkanContext::swapchain->GetImageViews()[i]);
        }

        descriptorProvider.FlushData();
    }
}

PostProcessStage::PostProcessStage()
{
    pipeline = Details::CreatePipeline();

    descriptorProvider = pipeline->CreateDescriptorProvider();
}

PostProcessStage::~PostProcessStage()
{
    RemoveScene();
}

void PostProcessStage::RegisterScene(const Scene* scene_)
{
    RemoveScene();

    scene = scene_;
    Assert(scene);

    const auto& renderComponent = scene->ctx().get<RenderContextComponent>();
    Details::CreateDescriptors(*descriptorProvider, *scene, renderComponent.gBuffer);
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

void PostProcessStage::Update() const
{
    Assert(scene);
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

    const glm::uvec3 groupCount = PipelineHelpers::CalculateWorkGroupCount(extent, Details::kWorkGroupSize);

    commandBuffer.dispatch(groupCount.x, groupCount.y, groupCount.z);
}

void PostProcessStage::Resize() const
{
    if (scene)
    {
        const auto& renderComponent = scene->ctx().get<RenderContextComponent>();
        Details::CreateDescriptors(*descriptorProvider, *scene, renderComponent.gBuffer);
    }
}

void PostProcessStage::ReloadShaders()
{
    Assert(scene);

    pipeline = Details::CreatePipeline();

    descriptorProvider = pipeline->CreateDescriptorProvider();

    const auto& renderComponent = scene->ctx().get<RenderContextComponent>();
    Details::CreateDescriptors(*descriptorProvider, *scene, renderComponent.gBuffer);
}

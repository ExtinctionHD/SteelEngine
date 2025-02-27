#include "Engine/Render/Stages/DebugDrawStage.hpp"

#include "Engine/Render/SceneRenderer.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Pipelines/ComputePipeline.hpp"
#include "Engine/Render/Vulkan/Pipelines/PipelineHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/ResourceContext.hpp"

namespace Details
{
    static int32_t imagePreviewIndex = 0;
    static CVarInt imagePreviewIndexCVar(
            "r.DebugDraw.ImagePreview.Index", imagePreviewIndex);

    // TODO implement r.DebugDraw.ImagePreview.Layer for Image3D

    static std::unique_ptr<ComputePipeline> CreatePipeline()
    {
        const ShaderModule shaderModule = VulkanContext::shaderManager->CreateComputeShaderModule(
                Filepath("~/Shaders/Debug/ImagePreview.comp"));

        std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(shaderModule);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

        return pipeline;
    }

    static std::vector<BaseImage> GetImagesForPreview(const SceneRenderContext& context)
    {
        std::vector<BaseImage> images;

        for (const auto& [image, sampler] : context.atmosphereLUTs.GetArray())
        {
            images.push_back(image);
        }

        return images;
    }

    static BaseImage SelectImageForPreview(const SceneRenderContext& context)
    {
        const std::vector<BaseImage> images = GetImagesForPreview(context);

        const int32_t maxIndex = static_cast<int32_t>(images.size()) - 1;

        const int32_t clampedIndex = std::clamp(imagePreviewIndex, 0, maxIndex);

        return images[clampedIndex];
    }

    static void CreateDescriptors(DescriptorProvider& descriptorProvider, const SceneRenderContext& context)
    {
        descriptorProvider.PushGlobalData("image", SelectImageForPreview(context).view);

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

DebugDrawStage::DebugDrawStage(const SceneRenderContext& context_)
    : RenderStage(context_)
{
    pipeline = Details::CreatePipeline();

    descriptorProvider = pipeline->CreateDescriptorProvider();
}

DebugDrawStage::~DebugDrawStage()
{
    DebugDrawStage::RemoveScene();
}

void DebugDrawStage::RegisterScene(const Scene* scene_)
{
    RenderStage::RegisterScene(scene_);

    Details::CreateDescriptors(*descriptorProvider, context);
}

void DebugDrawStage::RemoveScene()
{
    if (!scene)
    {
        return;
    }

    descriptorProvider->Clear();

    scene = nullptr;
}

void DebugDrawStage::Update()
{
    descriptorProvider->PushGlobalData("image", Details::SelectImageForPreview(context).view);

    descriptorProvider->FlushData();
}

void DebugDrawStage::Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const
{
    const vk::Image image = Details::SelectImageForPreview(context).image;

    const vk::Extent2D& extent = ResourceContext::GetImageDescription(image).extent;

    {
        const ImageLayoutTransition layoutTransition{
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::ImageLayout::eGeneral,
            PipelineBarrier::kFull,
        };

        ImageHelpers::TransitImageLayout(commandBuffer,
                image, ImageHelpers::kFlatColor, layoutTransition);
    }

    pipeline->Bind(commandBuffer);

    pipeline->BindDescriptorSets(commandBuffer, descriptorProvider->GetDescriptorSlice(imageIndex));

    const glm::uvec3 groupCount = PipelineHelpers::CalculateWorkGroupCount(extent);

    commandBuffer.dispatch(groupCount.x, groupCount.y, groupCount.z);

    {
        const ImageLayoutTransition layoutTransition{
            vk::ImageLayout::eGeneral,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            PipelineBarrier::kFull,
        };

        ImageHelpers::TransitImageLayout(commandBuffer,
                image, ImageHelpers::kFlatColor, layoutTransition);
    }
}

void DebugDrawStage::Resize()
{
    if (scene)
    {
        Details::CreateDescriptors(*descriptorProvider, context);
    }
}

void DebugDrawStage::ReloadShaders()
{
    pipeline = Details::CreatePipeline();

    descriptorProvider = pipeline->CreateDescriptorProvider();

    if (scene)
    {
        Details::CreateDescriptors(*descriptorProvider, context);
    }
}

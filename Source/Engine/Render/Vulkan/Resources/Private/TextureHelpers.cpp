#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Resources/DescriptorProvider.hpp"
#include "Engine/Render/Vulkan/Pipelines/PipelineHelpers.hpp"
#include "Engine/Render/Vulkan/Pipelines/ComputePipeline.hpp"

namespace Details
{
    static constexpr glm::uvec3 kWorkGroupSize(8, 8, 1);

    static const Filepath kShaderPath("~/Shaders/Compute/PanoramaToCube.comp");

    static std::unique_ptr<ComputePipeline> CreatePanoramaToCubePipeline()
    {
        const ShaderModule shaderModule = VulkanContext::shaderManager->CreateComputeShaderModule(
                kShaderPath, kWorkGroupSize);

        std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(shaderModule);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

        return pipeline;
    }
}

PanoramaToCube::PanoramaToCube()
{
    pipeline = Details::CreatePanoramaToCubePipeline();

    descriptorProvider = pipeline->CreateDescriptorProvider();
}

PanoramaToCube::~PanoramaToCube() = default;

void PanoramaToCube::Convert(const Texture& panoramaTexture,
        vk::Image cubeImage, const vk::Extent2D& cubeImageExtent) const
{
    const ImageHelpers::CubeFacesViews cubeFacesViews = ImageHelpers::CreateCubeFacesViews(cubeImage, 0);

    const TextureSampler panorama{ panoramaTexture.view, RenderContext::defaultSampler };

    descriptorProvider->PushGlobalData("panorama", panorama);

    for (const auto& cubeFaceView : cubeFacesViews)
    {
        descriptorProvider->PushSliceData("cubeFace", cubeFaceView);
    }

    descriptorProvider->FlushData();

    VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
        {
            {
                const ImageLayoutTransition layoutTransition{
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eGeneral,
                    PipelineBarrier{
                        SyncScope::kWaitForNone,
                        SyncScope::kComputeShaderWrite
                    }
                };

                ImageHelpers::TransitImageLayout(commandBuffer, cubeImage,
                        ImageHelpers::kCubeColor, layoutTransition);
            }

            pipeline->Bind(commandBuffer);

            pipeline->BindDescriptorSet(commandBuffer, 0, descriptorProvider->GetDescriptorSlice()[0]);

            const glm::uvec3 groupCount = PipelineHelpers::CalculateWorkGroupCount(
                    cubeImageExtent, Details::kWorkGroupSize);

            for (uint32_t faceIndex = 0; faceIndex < ImageHelpers::kCubeFaceCount; ++faceIndex)
            {
                pipeline->BindDescriptorSet(commandBuffer, 1, descriptorProvider->GetDescriptorSlice(faceIndex)[1]);

                pipeline->PushConstant(commandBuffer, "faceIndex", faceIndex);

                commandBuffer.dispatch(groupCount.x, groupCount.y, groupCount.z);
            }
        });

    descriptorProvider->Clear();

    for (const auto& view : cubeFacesViews)
    {
        VulkanContext::imageManager->DestroyImageView(cubeImage, view);
    }
}

std::vector<vk::ImageView> TextureHelpers::GetViews(const std::vector<Texture>& textures)
{
    std::vector<vk::ImageView> views(textures.size());

    for (size_t i = 0; i < textures.size(); ++i)
    {
        views[i] = textures[i].view;
    }

    return views;
}

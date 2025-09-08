#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Resources/DescriptorProvider.hpp"
#include "Engine/Render/Vulkan/Resources/ResourceContext.hpp"
#include "Engine/Render/Vulkan/Pipelines/PipelineHelpers.hpp"
#include "Engine/Render/Vulkan/Pipelines/ComputePipeline.hpp"

namespace Details
{
    static std::unique_ptr<ComputePipeline> CreatePanoramaToCubePipeline()
    {
        const ShaderModule shaderModule = VulkanContext::shaderManager->CreateComputeShaderModule(
                Filepath("~/Shaders/Compute/PanoramaToCube.comp"));

        std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(shaderModule);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

        return pipeline;
    }

    static auto GetTuple(const SamplerDescription& description)
    {
        return std::tie(description.magFilter, description.minFilter,
                description.mipmapMode, description.addressMode, description.maxAnisotropy,
                description.minLod, description.maxLod, description.unnormalizedCoords);
    }
}

bool SamplerDescription::operator==(const SamplerDescription& other) const
{
    return Details::GetTuple(*this) == Details::GetTuple(other);
}

bool SamplerDescription::operator<(const SamplerDescription& other) const
{
    return Details::GetTuple(*this) < Details::GetTuple(other);
}

PanoramaToCube::PanoramaToCube()
{
    pipeline = Details::CreatePanoramaToCubePipeline();
}

PanoramaToCube::~PanoramaToCube() = default;

BaseImage PanoramaToCube::GenerateCubeImage(const BaseImage& panoramaImage,
        vk::ImageUsageFlags usage, vk::ImageLayout finalLayout) const
{
    const ImageDescription& panoramaDescription
            = ResourceContext::GetImageDescription(panoramaImage.image);

    const vk::Extent2D panoramaExtent = panoramaDescription.extent;

    const vk::Extent2D cubeExtent(panoramaExtent.height / 2, panoramaExtent.height / 2);

    const CubeImageDescription description{
        .format = panoramaDescription.format,
        .extent = cubeExtent,
        .mipLevelCount = ImageHelpers::CalculateMipLevelCount(cubeExtent),
        .usage = usage,
    };

    const Texture panoramaTexture{ panoramaImage, TextureCache::GetSampler() };

    pipeline->PushGlobalData("panorama", &panoramaTexture);

    const BaseImage cubeImage = ResourceContext::CreateCubeImage(description);

    const CubeFaceViews cubeFaceViews = ResourceContext::CreateImageCubeFaceViews(cubeImage.image);

    for (const auto& cubeFaceView : cubeFaceViews)
    {
        pipeline->PushSliceData("cubeFace", cubeFaceView);
    }

    pipeline->FlushData();

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

                ImageHelpers::TransitImageLayout(commandBuffer, cubeImage.image,
                        ImageHelpers::GetSubresourceRange(description), layoutTransition);
            }

            pipeline->Bind(commandBuffer);

            const glm::uvec3 groupCount = PipelineHelpers::CalculateWorkGroupCount(cubeExtent);

            for (uint32_t faceIndex = 0; faceIndex < ImageHelpers::kCubeFaceCount; ++faceIndex)
            {
                pipeline->BindDescriptorSlice(commandBuffer, faceIndex);

                pipeline->PushConstant(commandBuffer, "faceIndex", faceIndex);

                commandBuffer.dispatch(groupCount.x, groupCount.y, groupCount.z);
            }

            ImageHelpers::GenerateMipLevels(commandBuffer, cubeImage.image, vk::ImageLayout::eGeneral, finalLayout);
        });

    for (const auto view : cubeFaceViews)
    {
        ResourceContext::DestroyResource(view);
    }

    return cubeImage;
}

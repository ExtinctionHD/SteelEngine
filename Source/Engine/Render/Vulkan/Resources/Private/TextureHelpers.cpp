#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Resources/DescriptorProvider.hpp"
#include "Engine/Render/Vulkan/Resources/ResourceContext.hpp"
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

    descriptorProvider = pipeline->CreateDescriptorProvider();
}

PanoramaToCube::~PanoramaToCube() = default;

CubeImage PanoramaToCube::GenerateCubeImage(const BaseImage& panoramaImage,
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

    descriptorProvider->PushGlobalData("panorama", &panoramaTexture);

    const CubeImage cubeImage = ResourceContext::CreateCubeImage(description);

    for (const auto& cubeFaceView : cubeImage.faceViews)
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

                ImageHelpers::TransitImageLayout(commandBuffer, cubeImage.image,
                        ImageHelpers::GetSubresourceRange(description), layoutTransition);
            }

            pipeline->Bind(commandBuffer);

            pipeline->BindDescriptorSet(commandBuffer, 0, descriptorProvider->GetDescriptorSlice()[0]);

            const glm::uvec3 groupCount = PipelineHelpers::CalculateWorkGroupCount(cubeExtent, Details::kWorkGroupSize);

            for (uint32_t faceIndex = 0; faceIndex < ImageHelpers::kCubeFaceCount; ++faceIndex)
            {
                pipeline->BindDescriptorSet(commandBuffer, 1, descriptorProvider->GetDescriptorSlice(faceIndex)[1]);

                pipeline->PushConstant(commandBuffer, "faceIndex", faceIndex);

                commandBuffer.dispatch(groupCount.x, groupCount.y, groupCount.z);
            }

            ImageHelpers::GenerateMipLevels(commandBuffer, cubeImage.image, vk::ImageLayout::eGeneral, finalLayout);
        });

    descriptorProvider->Clear();

    return cubeImage;
}

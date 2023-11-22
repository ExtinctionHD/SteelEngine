#include "Engine/Scene/ImageBasedLighting.hpp"

#include "Engine/Filesystem/Filepath.hpp"
#include "Engine/Render/RenderContext.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Resources/DescriptorProvider.hpp"
#include "Engine/Render/Vulkan/Pipelines/PipelineHelpers.hpp"
#include "Engine/Render/Vulkan/Pipelines/ComputePipeline.hpp"
#include "Engine/Render/Vulkan/Resources/ResourceContext.hpp"

#include "Utils/TimeHelpers.hpp"

namespace Details
{
    static constexpr glm::uvec3 kWorkGroupSize(8, 8, 1);

    static constexpr vk::Extent2D kSpecularLutExtent(256, 256);

    static constexpr vk::Extent2D kMaxIrradianceExtent(128, 128);

    static constexpr vk::Extent2D kMaxReflectionExtent(512, 512);

    static const Filepath kSpecularLutShaderPath("~/Shaders/Compute/ImageBasedLighting/SpecularLut.comp");
    static const Filepath kIrradianceShaderPath("~/Shaders/Compute/ImageBasedLighting/Irradiance.comp");
    static const Filepath kReflectionShaderPath("~/Shaders/Compute/ImageBasedLighting/Reflection.comp");

    static const SamplerDescription kIrradianceSamplerDescription{
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eNearest,
        .addressMode = vk::SamplerAddressMode::eRepeat,
        .maxAnisotropy = 0.0f,
        .minLod = 0.0f,
        .maxLod = 0.0f,
    };

    static const SamplerDescription kReflectionSamplerDescription{
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eLinear,
        .addressMode = vk::SamplerAddressMode::eRepeat,
        .maxAnisotropy = 0.0f,
    };

    static const SamplerDescription kSpecularLutSamplerDescription{
        .magFilter = vk::Filter::eNearest,
        .minFilter = vk::Filter::eNearest,
        .mipmapMode = vk::SamplerMipmapMode::eNearest,
        .addressMode = vk::SamplerAddressMode::eClampToEdge,
        .maxAnisotropy = 0.0f,
        .minLod = 0.0f,
        .maxLod = 0.0f,
    };

    static std::unique_ptr<ComputePipeline> CreateIrradiancePipeline()
    {
        const ShaderModule shaderModule = VulkanContext::shaderManager->CreateComputeShaderModule(
                kIrradianceShaderPath, kWorkGroupSize);

        std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(shaderModule);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

        return pipeline;
    }

    static std::unique_ptr<ComputePipeline> CreateReflectionPipeline()
    {
        const ShaderModule shaderModule = VulkanContext::shaderManager->CreateComputeShaderModule(
                kReflectionShaderPath, kWorkGroupSize);

        std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(shaderModule);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

        return pipeline;
    }

    static BaseImage CreateSpecularLutImage()
    {
        constexpr vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled;

        return ResourceContext::CreateBaseImage({
            .format = vk::Format::eR16G16Sfloat,
            .extent = kSpecularLutExtent,
            .usage = usage
        });
    }

    static Texture GenerateSpecularLut()
    {
        const BaseImage specularLut = CreateSpecularLutImage();

        const ShaderModule shaderModule = VulkanContext::shaderManager->CreateComputeShaderModule(
                kSpecularLutShaderPath, kWorkGroupSize);

        const std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(shaderModule);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

        const std::unique_ptr<DescriptorProvider> descriptorProvider = pipeline->CreateDescriptorProvider();

        descriptorProvider->PushGlobalData("specularLut", specularLut.view);
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

                    ImageHelpers::TransitImageLayout(commandBuffer, specularLut.image,
                            ImageHelpers::kFlatColor, layoutTransition);
                }

                pipeline->Bind(commandBuffer);

                pipeline->BindDescriptorSets(commandBuffer, descriptorProvider->GetDescriptorSlice());

                const glm::uvec3 groupCount = PipelineHelpers::CalculateWorkGroupCount(
                        kSpecularLutExtent, Details::kWorkGroupSize);

                commandBuffer.dispatch(groupCount.x, groupCount.y, groupCount.z);

                {
                    const ImageLayoutTransition layoutTransition{
                        vk::ImageLayout::eGeneral,
                        vk::ImageLayout::eShaderReadOnlyOptimal,
                        PipelineBarrier{
                            SyncScope::kComputeShaderWrite,
                            SyncScope::kBlockNone
                        }
                    };

                    ImageHelpers::TransitImageLayout(commandBuffer, specularLut.image,
                            ImageHelpers::kFlatColor, layoutTransition);
                }
            });

        VulkanHelpers::SetObjectName(VulkanContext::device->Get(), specularLut.image, "SpecularLut");

        return Texture{ specularLut, TextureCache::GetSampler(kSpecularLutSamplerDescription) };
    }

    static const vk::Extent2D& GetIrradianceExtent(const vk::Extent2D& cubemapExtent)
    {
        if (cubemapExtent.width <= kMaxIrradianceExtent.width)
        {
            return cubemapExtent;
        }

        return kMaxIrradianceExtent;
    }

    static const vk::Extent2D& GetReflectionExtent(const vk::Extent2D& cubemapExtent)
    {
        if (cubemapExtent.width <= kMaxReflectionExtent.width)
        {
            return cubemapExtent;
        }

        return kMaxReflectionExtent;
    }

    static CubeImage CreateIrradianceImage(vk::Format format, const vk::Extent2D& extent)
    {
        constexpr vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled;

        return ResourceContext::CreateCubeImage({
            .format = format,
            .extent = extent,
            .usage = usage
        });
    }

    static CubeImage CreateReflectionImage(vk::Format format, const vk::Extent2D& extent)
    {
        constexpr vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled;

        return ResourceContext::CreateCubeImage({
            .format = format,
            .extent = extent,
            .mipLevelCount = ImageHelpers::CalculateMipLevelCount(extent),
            .usage = usage
        });
    }

    static std::vector<CubeFaceViews> CreateReflectionFaceViews(CubeImage cubeImage, uint32_t mipLevelCount)
    {
        std::vector<CubeFaceViews> reflectionViews(mipLevelCount);

        for (uint32_t mipLevel = 0; mipLevel < mipLevelCount; ++mipLevel)
        {
            for (uint32_t faceIndex = 0; faceIndex < reflectionViews[mipLevel].size(); ++faceIndex)
            {
                const vk::ImageSubresourceRange subresourceRange(
                        vk::ImageAspectFlagBits::eColor, mipLevel, 1, faceIndex, 1);

                reflectionViews[mipLevel][faceIndex] = ResourceContext::CreateImageView({
                    .image = cubeImage.image,
                    .viewType = vk::ImageViewType::e2D,
                    .subresourceRange = subresourceRange
                });
            }
        }

        return reflectionViews;
    }
}

ImageBasedLighting::ImageBasedLighting()
{
    irradiancePipeline = Details::CreateIrradiancePipeline();
    reflectionPipeline = Details::CreateReflectionPipeline();

    specularLut = Details::GenerateSpecularLut();
}

ImageBasedLighting::~ImageBasedLighting()
{
    ResourceContext::DestroyResource(specularLut.image);
}

Texture ImageBasedLighting::GenerateIrradiance(const Texture& cubemap) const
{
    EASY_FUNCTION()

    const ImageDescription& cubemapDescription
            = ResourceContext::GetImageDescription(cubemap.image.image);

    const vk::Extent2D irradianceExtent = Details::GetIrradianceExtent(cubemapDescription.extent);

    const CubeImage irradianceImage = Details::CreateIrradianceImage(cubemapDescription.format, irradianceExtent);

    const std::unique_ptr<DescriptorProvider> descriptorProvider = irradiancePipeline->CreateDescriptorProvider();

    descriptorProvider->PushGlobalData("environmentMap", &cubemap);

    for (const auto& irradianceFaceView : irradianceImage.faceViews)
    {
        descriptorProvider->PushSliceData("irradianceFace", irradianceFaceView);
    }

    descriptorProvider->FlushData();

    for (uint32_t faceIndex = 0; faceIndex < ImageHelpers::kCubeFaceCount; ++faceIndex)
    {
        VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
            {
                if (faceIndex == 0)
                {
                    const ImageLayoutTransition layoutTransition{
                        vk::ImageLayout::eUndefined,
                        vk::ImageLayout::eGeneral,
                        PipelineBarrier{
                            SyncScope::kWaitForNone,
                            SyncScope::kComputeShaderWrite
                        }
                    };

                    ImageHelpers::TransitImageLayout(commandBuffer, irradianceImage.image,
                            ImageHelpers::kCubeColor, layoutTransition);
                }

                irradiancePipeline->Bind(commandBuffer);

                irradiancePipeline->BindDescriptorSets(commandBuffer,
                        descriptorProvider->GetDescriptorSlice(faceIndex));

                irradiancePipeline->PushConstant(commandBuffer, "faceIndex", faceIndex);

                const glm::uvec3 groupCount = PipelineHelpers::CalculateWorkGroupCount(
                        irradianceExtent, Details::kWorkGroupSize);

                commandBuffer.dispatch(groupCount.x, groupCount.y, groupCount.z);

                if (faceIndex == ImageHelpers::kCubeFaceCount - 1)
                {
                    const ImageLayoutTransition layoutTransition{
                        vk::ImageLayout::eGeneral,
                        vk::ImageLayout::eShaderReadOnlyOptimal,
                        PipelineBarrier{
                            SyncScope::kComputeShaderWrite,
                            SyncScope::kBlockNone
                        }
                    };

                    ImageHelpers::TransitImageLayout(commandBuffer, irradianceImage.image,
                            ImageHelpers::kCubeColor, layoutTransition);
                }
            });
    }

    descriptorProvider->Clear();

    VulkanHelpers::SetObjectName(VulkanContext::device->Get(), irradianceImage.image, "IrradianceMap");

    return Texture{ irradianceImage, TextureCache::GetSampler(Details::kIrradianceSamplerDescription) };
}

Texture ImageBasedLighting::GenerateReflection(const Texture& cubemap) const
{
    EASY_FUNCTION()

    const ImageDescription& cubemapDescription
            = ResourceContext::GetImageDescription(cubemap.image.image);

    const vk::Extent2D reflectionExtent
            = Details::GetReflectionExtent(cubemapDescription.extent);

    const CubeImage reflectionImage
            = Details::CreateReflectionImage(cubemapDescription.format, reflectionExtent);

    const ImageDescription reflectionDescription
            = ResourceContext::GetImageDescription(reflectionImage.image);

    const std::vector<CubeFaceViews> reflectionFaceViews
            = Details::CreateReflectionFaceViews(reflectionImage, reflectionDescription.mipLevelCount);

    const std::unique_ptr<DescriptorProvider> descriptorProvider = reflectionPipeline->CreateDescriptorProvider();

    descriptorProvider->PushGlobalData("environmentMap", &cubemap);

    for (const auto& reflectionMipLevelFaceViews : reflectionFaceViews)
    {
        for (const auto& faceView : reflectionMipLevelFaceViews)
        {
            descriptorProvider->PushSliceData("reflectionFace", faceView);
        }
    }

    descriptorProvider->FlushData();

    const vk::ImageSubresourceRange reflectionSubresourceRange
            = ImageHelpers::GetSubresourceRange(reflectionDescription);

    for (uint32_t mipLevel = 0; mipLevel < reflectionDescription.mipLevelCount; ++mipLevel)
    {
        VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
            {
                if (mipLevel == 0)
                {
                    const ImageLayoutTransition layoutTransition{
                        vk::ImageLayout::eUndefined,
                        vk::ImageLayout::eGeneral,
                        PipelineBarrier{
                            SyncScope::kWaitForNone,
                            SyncScope::kComputeShaderWrite
                        }
                    };

                    ImageHelpers::TransitImageLayout(commandBuffer, reflectionImage.image,
                            reflectionSubresourceRange, layoutTransition);
                }

                reflectionPipeline->Bind(commandBuffer);

                reflectionPipeline->BindDescriptorSet(commandBuffer, 0, descriptorProvider->GetDescriptorSlice()[0]);

                for (uint32_t faceIndex = 0; faceIndex < ImageHelpers::kCubeFaceCount; ++faceIndex)
                {
                    const uint32_t sliceIndex = mipLevel * ImageHelpers::kCubeFaceCount + faceIndex;

                    reflectionPipeline->BindDescriptorSet(commandBuffer, 1,
                            descriptorProvider->GetDescriptorSlice(sliceIndex)[1]);

                    const float maxMipLevel = static_cast<float>(reflectionDescription.mipLevelCount - 1);
                    const float roughness = static_cast<float>(mipLevel) / maxMipLevel;

                    reflectionPipeline->PushConstant(commandBuffer, "roughness", roughness);
                    reflectionPipeline->PushConstant(commandBuffer, "faceIndex", faceIndex);

                    const vk::Extent2D mipLevelExtent
                            = ImageHelpers::CalculateMipLevelExtent(reflectionExtent, mipLevel);

                    const glm::uvec3 groupCount
                            = PipelineHelpers::CalculateWorkGroupCount(mipLevelExtent, Details::kWorkGroupSize);

                    commandBuffer.dispatch(groupCount.x, groupCount.y, groupCount.z);
                }

                if (mipLevel == reflectionDescription.mipLevelCount - 1)
                {
                    const ImageLayoutTransition layoutTransition{
                        vk::ImageLayout::eGeneral,
                        vk::ImageLayout::eShaderReadOnlyOptimal,
                        PipelineBarrier{
                            SyncScope::kComputeShaderWrite,
                            SyncScope::kBlockNone
                        }
                    };

                    ImageHelpers::TransitImageLayout(commandBuffer, reflectionImage.image,
                            reflectionSubresourceRange, layoutTransition);
                }
            });
    }

    descriptorProvider->Clear();

    VulkanHelpers::SetObjectName(VulkanContext::device->Get(), reflectionImage.image, "ReflectionMap");

    return Texture{ reflectionImage, TextureCache::GetSampler(Details::kReflectionSamplerDescription) };
}

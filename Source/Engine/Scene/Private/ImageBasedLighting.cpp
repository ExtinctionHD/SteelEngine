#include "Engine/Scene/ImageBasedLighting.hpp"

#include "Engine/Filesystem/Filepath.hpp"
#include "Engine/Render/RenderContext.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"
#include "Engine/Render/Vulkan/Resources/DescriptorProvider.hpp"
#include "Engine/Render/Vulkan/Pipelines/PipelineHelpers.hpp"
#include "Engine/Render/Vulkan/Pipelines/ComputePipeline.hpp"
#include "Engine/Render/Vulkan/Resources/ResourceContext.hpp"

#include "Utils/TimeHelpers.hpp"

namespace Details
{
    static constexpr glm::uvec3 kWorkGroupSize(8, 8, 1);

    static constexpr vk::Extent2D kSpecularBRDFExtent(256, 256);

    static constexpr vk::Extent2D kMaxIrradianceExtent(128, 128);

    static constexpr vk::Extent2D kMaxReflectionExtent(512, 512);

    static const Filepath kSpecularBRDFShaderPath("~/Shaders/Compute/ImageBasedLighting/SpecularBRDF.comp");
    static const Filepath kIrradianceShaderPath("~/Shaders/Compute/ImageBasedLighting/Irradiance.comp");
    static const Filepath kReflectionShaderPath("~/Shaders/Compute/ImageBasedLighting/Reflection.comp");

    static ImageBasedLighting::Samplers CreateSamplers()
    {
        const SamplerDescription irradianceDescription{
            vk::Filter::eLinear,
            vk::Filter::eLinear,
            vk::SamplerMipmapMode::eNearest,
            vk::SamplerAddressMode::eRepeat,
            std::nullopt,
            0.0f, 0.0f,
            false
        };

        const SamplerDescription reflectionDescription{
            vk::Filter::eLinear,
            vk::Filter::eLinear,
            vk::SamplerMipmapMode::eLinear,
            vk::SamplerAddressMode::eRepeat,
            std::nullopt,
            0.0f, std::numeric_limits<float>::max(),
            false
        };

        const SamplerDescription specularBRDFDescription{
            vk::Filter::eNearest,
            vk::Filter::eNearest,
            vk::SamplerMipmapMode::eNearest,
            vk::SamplerAddressMode::eClampToEdge,
            std::nullopt,
            0.0f, 0.0f,
            false
        };

        const ImageBasedLighting::Samplers samplers{
            VulkanContext::textureManager->CreateSampler(irradianceDescription),
            VulkanContext::textureManager->CreateSampler(reflectionDescription),
            VulkanContext::textureManager->CreateSampler(specularBRDFDescription),
        };

        return samplers;
    }

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

    static BaseImage CreateSpecularBRDFImage()
    {
        constexpr vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled;

        return ResourceContext::CreateBaseImage({
            .format = vk::Format::eR16G16Sfloat,
            .extent = kSpecularBRDFExtent,
            .usage = usage
        });
    }

    static BaseImage GenerateSpecularBRDF()
    {
        const BaseImage specularBRDF = CreateSpecularBRDFImage();

        const ShaderModule shaderModule = VulkanContext::shaderManager->CreateComputeShaderModule(
                kSpecularBRDFShaderPath, kWorkGroupSize);

        const std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(shaderModule);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

        const std::unique_ptr<DescriptorProvider> descriptorProvider = pipeline->CreateDescriptorProvider();

        descriptorProvider->PushGlobalData("specularBRDF", specularBRDF.view);
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

                    ImageHelpers::TransitImageLayout(commandBuffer, specularBRDF.image,
                            ImageHelpers::kFlatColor, layoutTransition);
                }

                pipeline->Bind(commandBuffer);

                pipeline->BindDescriptorSets(commandBuffer, descriptorProvider->GetDescriptorSlice());

                const glm::uvec3 groupCount = PipelineHelpers::CalculateWorkGroupCount(
                        kSpecularBRDFExtent, Details::kWorkGroupSize);

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

                    ImageHelpers::TransitImageLayout(commandBuffer, specularBRDF.image,
                            ImageHelpers::kFlatColor, layoutTransition);
                }
            });

        VulkanHelpers::SetObjectName(VulkanContext::device->Get(), specularBRDF.image, "SpecularBRDF");

        return specularBRDF;
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

    specularBRDF = Details::GenerateSpecularBRDF();

    samplers = Details::CreateSamplers();
}

ImageBasedLighting::~ImageBasedLighting()
{
    ResourceContext::DestroyResource(specularBRDF);

    ResourceContext::DestroyResource(samplers.specularBRDF);
    ResourceContext::DestroyResource(samplers.irradiance);
    ResourceContext::DestroyResource(samplers.reflection);
}

CubeImage ImageBasedLighting::GenerateIrradianceImage(const CubeImage& cubemapImage) const
{
    EASY_FUNCTION()

    const ImageDescription& description = ResourceContext::GetImageDescription(cubemapImage.image);

    const vk::Extent2D irradianceExtent = Details::GetIrradianceExtent(description.extent);

    const CubeImage irradianceImage = Details::CreateIrradianceImage(description.format, irradianceExtent);

    const std::unique_ptr<DescriptorProvider> descriptorProvider = irradiancePipeline->CreateDescriptorProvider();

    const ViewSampler environmentMap{ cubemapImage.cubeView, RenderContext::defaultSampler };

    descriptorProvider->PushGlobalData("environmentMap", environmentMap);

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

    return irradianceImage;
}

CubeImage ImageBasedLighting::GenerateReflectionImage(const CubeImage& cubemapTexture) const
{
    EASY_FUNCTION()

    const ImageDescription& cubemapDescription
            = ResourceContext::GetImageDescription(cubemapTexture.image);

    const vk::Extent2D reflectionExtent
            = Details::GetReflectionExtent(cubemapDescription.extent);

    const CubeImage reflectionImage
            = Details::CreateReflectionImage(cubemapDescription.format, reflectionExtent);

    const ImageDescription reflectionDescription
            = ResourceContext::GetImageDescription(reflectionImage.image);

    const std::vector<CubeFaceViews> reflectionFaceViews
            = Details::CreateReflectionFaceViews(reflectionImage, reflectionDescription.mipLevelCount);

    const std::unique_ptr<DescriptorProvider> descriptorProvider = reflectionPipeline->CreateDescriptorProvider();

    const ViewSampler environmentMap{ cubemapTexture.cubeView, RenderContext::defaultSampler };

    descriptorProvider->PushGlobalData("environmentMap", environmentMap);

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

    return reflectionImage;
}

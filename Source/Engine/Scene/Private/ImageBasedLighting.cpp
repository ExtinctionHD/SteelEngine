#include "Engine/Scene/ImageBasedLighting.hpp"

#include "Engine/Filesystem/Filepath.hpp"
#include "Engine/Render/RenderContext.hpp"
#include "Engine/Render/Vulkan/Pipelines/ComputePipeline.hpp"
#include "Engine/Render/Vulkan/Pipelines/PipelineHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/DescriptorProvider.hpp"
#include "Engine/Render/Vulkan/Resources/ResourceHelpers.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"

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
        const SamplerDescription irradianceDescription{ vk::Filter::eLinear,
            vk::Filter::eLinear,
            vk::SamplerMipmapMode::eNearest,
            vk::SamplerAddressMode::eRepeat,
            std::nullopt,
            0.0f,
            0.0f,
            false };

        const SamplerDescription reflectionDescription{ vk::Filter::eLinear,
            vk::Filter::eLinear,
            vk::SamplerMipmapMode::eLinear,
            vk::SamplerAddressMode::eRepeat,
            std::nullopt,
            0.0f,
            std::numeric_limits<float>::max(),
            false };

        const SamplerDescription specularBRDFDescription{ vk::Filter::eNearest,
            vk::Filter::eNearest,
            vk::SamplerMipmapMode::eNearest,
            vk::SamplerAddressMode::eClampToEdge,
            std::nullopt,
            0.0f,
            0.0f,
            false };

        const ImageBasedLighting::Samplers samplers{
            VulkanContext::textureManager->CreateSampler(irradianceDescription),
            VulkanContext::textureManager->CreateSampler(reflectionDescription),
            VulkanContext::textureManager->CreateSampler(specularBRDFDescription),
        };

        return samplers;
    }

    static std::unique_ptr<ComputePipeline> CreateIrradiancePipeline()
    {
        const ShaderModule shaderModule
            = VulkanContext::shaderManager->CreateComputeShaderModule(kIrradianceShaderPath, kWorkGroupSize);

        std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(shaderModule);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

        return pipeline;
    }

    static std::unique_ptr<ComputePipeline> CreateReflectionPipeline()
    {
        const ShaderModule shaderModule
            = VulkanContext::shaderManager->CreateComputeShaderModule(kReflectionShaderPath, kWorkGroupSize);

        std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(shaderModule);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

        return pipeline;
    }

    static Texture CreateSpecularBRDFTexture()
    {
        const vk::ImageUsageFlags imageUsage = vk::ImageUsageFlagBits::eTransferDst
            | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled;

        const ImageDescription imageDescription{ ImageType::e2D,
            vk::Format::eR16G16Sfloat,
            VulkanHelpers::GetExtent3D(kSpecularBRDFExtent),
            1,
            1,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            imageUsage,
            vk::MemoryPropertyFlagBits::eDeviceLocal };

        const vk::Image image
            = VulkanContext::imageManager->CreateImage(imageDescription, ImageCreateFlags::kNone);

        const vk::ImageView view = VulkanContext::imageManager->CreateView(
            image, vk::ImageViewType::e2D, ImageHelpers::kFlatColor);

        return Texture{ image, view };
    }

    static Texture GenerateSpecularBRDF()
    {
        const Texture specularBRDF = CreateSpecularBRDFTexture();

        const ShaderModule shaderModule = VulkanContext::shaderManager->CreateComputeShaderModule(
            kSpecularBRDFShaderPath, kWorkGroupSize);

        const std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(shaderModule);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

        const std::unique_ptr<DescriptorProvider> descriptorProvider = pipeline->CreateDescriptorProvider();

        descriptorProvider->PushGlobalData("specularBRDF", specularBRDF.view);
        descriptorProvider->FlushData();

        VulkanContext::device->ExecuteOneTimeCommands(
            [&](vk::CommandBuffer commandBuffer)
            {
                {
                    const ImageLayoutTransition layoutTransition{ vk::ImageLayout::eUndefined,
                        vk::ImageLayout::eGeneral,
                        PipelineBarrier{ SyncScope::kWaitForNone, SyncScope::kComputeShaderWrite } };

                    ImageHelpers::TransitImageLayout(
                        commandBuffer, specularBRDF.image, ImageHelpers::kFlatColor, layoutTransition);
                }

                pipeline->Bind(commandBuffer);

                pipeline->BindDescriptorSets(commandBuffer, descriptorProvider->GetDescriptorSlice());

                const glm::uvec3 groupCount
                    = PipelineHelpers::CalculateWorkGroupCount(kSpecularBRDFExtent, Details::kWorkGroupSize);

                commandBuffer.dispatch(groupCount.x, groupCount.y, groupCount.z);

                {
                    const ImageLayoutTransition layoutTransition{ vk::ImageLayout::eGeneral,
                        vk::ImageLayout::eShaderReadOnlyOptimal,
                        PipelineBarrier{ SyncScope::kComputeShaderWrite, SyncScope::kBlockNone } };

                    ImageHelpers::TransitImageLayout(
                        commandBuffer, specularBRDF.image, ImageHelpers::kFlatColor, layoutTransition);
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

    static vk::Image CreateIrradianceImage(vk::Format format, const vk::Extent2D& extent)
    {
        constexpr vk::ImageUsageFlags usage
            = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled;

        const ImageDescription imageDescription{ ImageType::eCube,
            format,
            VulkanHelpers::GetExtent3D(extent),
            1,
            ImageHelpers::kCubeFaceCount,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            usage,
            vk::MemoryPropertyFlagBits::eDeviceLocal };

        return VulkanContext::imageManager->CreateImage(imageDescription, ImageCreateFlags::kNone);
    }

    static vk::Image CreateReflectionImage(vk::Format format, const vk::Extent2D& extent)
    {
        constexpr vk::ImageUsageFlags usage
            = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled;

        const ImageDescription imageDescription{ ImageType::eCube,
            format,
            VulkanHelpers::GetExtent3D(extent),
            ImageHelpers::CalculateMipLevelCount(extent),
            ImageHelpers::kCubeFaceCount,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            usage,
            vk::MemoryPropertyFlagBits::eDeviceLocal };

        return VulkanContext::imageManager->CreateImage(imageDescription, ImageCreateFlags::kNone);
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
    ResourceHelpers::DestroyResource(specularBRDF);

    ResourceHelpers::DestroyResource(samplers.specularBRDF);
    ResourceHelpers::DestroyResource(samplers.irradiance);
    ResourceHelpers::DestroyResource(samplers.reflection);
}

Texture ImageBasedLighting::GenerateIrradianceTexture(const Texture& cubemapTexture) const
{
    EASY_FUNCTION()

    const ImageDescription& cubemapDescription
        = VulkanContext::imageManager->GetImageDescription(cubemapTexture.image);

    const vk::Extent2D cubemapExtent = VulkanHelpers::GetExtent2D(cubemapDescription.extent);
    const vk::Extent2D irradianceExtent = Details::GetIrradianceExtent(cubemapExtent);

    const vk::Image irradianceImage
        = Details::CreateIrradianceImage(cubemapDescription.format, irradianceExtent);
    const ImageHelpers::CubeFacesViews irradianceFacesViews
        = ImageHelpers::CreateCubeFacesViews(irradianceImage, 0);

    const std::unique_ptr<DescriptorProvider> descriptorProvider
        = irradiancePipeline->CreateDescriptorProvider();

    const TextureSampler environmentMap{ cubemapTexture.view, RenderContext::defaultSampler };

    descriptorProvider->PushGlobalData("environmentMap", environmentMap);

    for (const auto& irradianceFaceView : irradianceFacesViews)
    {
        descriptorProvider->PushSliceData("irradianceFace", irradianceFaceView);
    }

    descriptorProvider->FlushData();

    for (uint32_t faceIndex = 0; faceIndex < ImageHelpers::kCubeFaceCount; ++faceIndex)
    {
        VulkanContext::device->ExecuteOneTimeCommands(
            [&](vk::CommandBuffer commandBuffer)
            {
                if (faceIndex == 0)
                {
                    const ImageLayoutTransition layoutTransition{ vk::ImageLayout::eUndefined,
                        vk::ImageLayout::eGeneral,
                        PipelineBarrier{ SyncScope::kWaitForNone, SyncScope::kComputeShaderWrite } };

                    ImageHelpers::TransitImageLayout(
                        commandBuffer, irradianceImage, ImageHelpers::kCubeColor, layoutTransition);
                }

                irradiancePipeline->Bind(commandBuffer);

                irradiancePipeline->BindDescriptorSets(
                    commandBuffer, descriptorProvider->GetDescriptorSlice(faceIndex));

                irradiancePipeline->PushConstant(commandBuffer, "faceIndex", faceIndex);

                const glm::uvec3 groupCount
                    = PipelineHelpers::CalculateWorkGroupCount(irradianceExtent, Details::kWorkGroupSize);

                commandBuffer.dispatch(groupCount.x, groupCount.y, groupCount.z);

                if (faceIndex == ImageHelpers::kCubeFaceCount - 1)
                {
                    const ImageLayoutTransition layoutTransition{ vk::ImageLayout::eGeneral,
                        vk::ImageLayout::eShaderReadOnlyOptimal,
                        PipelineBarrier{ SyncScope::kComputeShaderWrite, SyncScope::kBlockNone } };

                    ImageHelpers::TransitImageLayout(
                        commandBuffer, irradianceImage, ImageHelpers::kCubeColor, layoutTransition);
                }
            });
    }

    descriptorProvider->Clear();

    for (const auto& view : irradianceFacesViews)
    {
        VulkanContext::imageManager->DestroyImageView(irradianceImage, view);
    }

    const vk::ImageView irradianceView = VulkanContext::imageManager->CreateView(
        irradianceImage, vk::ImageViewType::eCube, ImageHelpers::kCubeColor);

    VulkanHelpers::SetObjectName(VulkanContext::device->Get(), irradianceImage, "IrradianceMap");

    return Texture{ irradianceImage, irradianceView };
}

Texture ImageBasedLighting::GenerateReflectionTexture(const Texture& cubemapTexture) const
{
    EASY_FUNCTION()

    const ImageDescription& cubemapDescription
        = VulkanContext::imageManager->GetImageDescription(cubemapTexture.image);

    const vk::Extent2D cubemapExtent = VulkanHelpers::GetExtent2D(cubemapDescription.extent);
    const vk::Extent2D reflectionExtent = Details::GetReflectionExtent(cubemapExtent);

    const vk::Image reflectionImage
        = Details::CreateReflectionImage(cubemapDescription.format, reflectionExtent);

    const uint32_t reflectionMipLevelCount = ImageHelpers::CalculateMipLevelCount(reflectionExtent);
    std::vector<ImageHelpers::CubeFacesViews> reflectionFacesViewsForMipLevels(reflectionMipLevelCount);
    for (uint32_t i = 0; i < reflectionFacesViewsForMipLevels.size(); ++i)
    {
        reflectionFacesViewsForMipLevels[i] = ImageHelpers::CreateCubeFacesViews(reflectionImage, i);
    }

    const std::unique_ptr<DescriptorProvider> descriptorProvider
        = reflectionPipeline->CreateDescriptorProvider();

    const TextureSampler environmentMap{ cubemapTexture.view, RenderContext::defaultSampler };

    descriptorProvider->PushGlobalData("environmentMap", environmentMap);

    for (const auto& reflectionFacesViews : reflectionFacesViewsForMipLevels)
    {
        for (const auto& faceView : reflectionFacesViews)
        {
            descriptorProvider->PushSliceData("reflectionFace", faceView);
        }
    }

    descriptorProvider->FlushData();

    const vk::ImageSubresourceRange reflectionSubresourceRange(
        vk::ImageAspectFlagBits::eColor, 0, reflectionMipLevelCount, 0, ImageHelpers::kCubeFaceCount);

    for (uint32_t mipLevel = 0; mipLevel < reflectionMipLevelCount; ++mipLevel)
    {
        VulkanContext::device->ExecuteOneTimeCommands(
            [&](vk::CommandBuffer commandBuffer)
            {
                if (mipLevel == 0)
                {
                    const ImageLayoutTransition layoutTransition{ vk::ImageLayout::eUndefined,
                        vk::ImageLayout::eGeneral,
                        PipelineBarrier{ SyncScope::kWaitForNone, SyncScope::kComputeShaderWrite } };

                    ImageHelpers::TransitImageLayout(
                        commandBuffer, reflectionImage, reflectionSubresourceRange, layoutTransition);
                }

                reflectionPipeline->Bind(commandBuffer);

                reflectionPipeline->BindDescriptorSet(
                    commandBuffer, 0, descriptorProvider->GetDescriptorSlice()[0]);

                for (uint32_t faceIndex = 0; faceIndex < ImageHelpers::kCubeFaceCount; ++faceIndex)
                {
                    const uint32_t sliceIndex = mipLevel * ImageHelpers::kCubeFaceCount + faceIndex;

                    reflectionPipeline->BindDescriptorSet(
                        commandBuffer, 1, descriptorProvider->GetDescriptorSlice(sliceIndex)[1]);

                    const float maxMipLevel = static_cast<float>(reflectionMipLevelCount - 1);
                    const float roughness = static_cast<float>(mipLevel) / maxMipLevel;

                    reflectionPipeline->PushConstant(commandBuffer, "roughness", roughness);
                    reflectionPipeline->PushConstant(commandBuffer, "faceIndex", faceIndex);

                    const vk::Extent2D mipLevelExtent
                        = ImageHelpers::CalculateMipLevelExtent(reflectionExtent, mipLevel);

                    const glm::uvec3 groupCount
                        = PipelineHelpers::CalculateWorkGroupCount(mipLevelExtent, Details::kWorkGroupSize);

                    commandBuffer.dispatch(groupCount.x, groupCount.y, groupCount.z);
                }

                if (mipLevel == reflectionMipLevelCount - 1)
                {
                    const ImageLayoutTransition layoutTransition{ vk::ImageLayout::eGeneral,
                        vk::ImageLayout::eShaderReadOnlyOptimal,
                        PipelineBarrier{ SyncScope::kComputeShaderWrite, SyncScope::kBlockNone } };

                    ImageHelpers::TransitImageLayout(
                        commandBuffer, reflectionImage, reflectionSubresourceRange, layoutTransition);
                }
            });
    }

    descriptorProvider->Clear();

    for (const auto& reflectionFacesViews : reflectionFacesViewsForMipLevels)
    {
        for (const auto& view : reflectionFacesViews)
        {
            VulkanContext::imageManager->DestroyImageView(reflectionImage, view);
        }
    }

    const vk::ImageView reflectionView = VulkanContext::imageManager->CreateView(
        reflectionImage, vk::ImageViewType::eCube, reflectionSubresourceRange);

    VulkanHelpers::SetObjectName(VulkanContext::device->Get(), reflectionImage, "ReflectionMap");

    return Texture{ reflectionImage, reflectionView };
}

#include "Engine/Scene/ImageBasedLighting.hpp"

#include "Engine/Filesystem/Filepath.hpp"
#include "Engine/Render/Renderer.hpp"
#include "Engine/Render/Vulkan/ComputeHelpers.hpp"
#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/ComputePipeline.hpp"

namespace Details
{
    constexpr glm::uvec2 kWorkGroupSize(16, 16);

    constexpr vk::Extent2D kMaxIrradianceExtent(512, 512);

    constexpr vk::Extent2D kMaxReflectionExtent(256, 256);

    const Filepath kIrradianceShaderPath("~/Shaders/Compute/ImageBasedLighting/Irradiance.comp");
    const Filepath kReflectionShaderPath("~/Shaders/Compute/ImageBasedLighting/Reflection.comp");

    ImageBasedLighting::Samplers CreateSamplers()
    {
        const SamplerDescription specularBRDFDescription{
            vk::Filter::eNearest,
            vk::Filter::eNearest,
            vk::SamplerMipmapMode::eNearest,
            vk::SamplerAddressMode::eClampToEdge,
            1.0f, 0.0f, 0.0f
        };

        const SamplerDescription irradianceDescription{
            vk::Filter::eLinear,
            vk::Filter::eLinear,
            vk::SamplerMipmapMode::eNearest,
            vk::SamplerAddressMode::eRepeat,
            1.0f, 0.0f, 0.0f
        };

        const SamplerDescription reflectionDescription{
            vk::Filter::eLinear,
            vk::Filter::eLinear,
            vk::SamplerMipmapMode::eLinear,
            vk::SamplerAddressMode::eRepeat,
            1.0f, 0.0f, 0.0f
        };

        const ImageBasedLighting::Samplers samplers{
            VulkanContext::textureManager->CreateSampler(specularBRDFDescription),
            VulkanContext::textureManager->CreateSampler(irradianceDescription),
            VulkanContext::textureManager->CreateSampler(reflectionDescription),
        };

        return samplers;
    }

    vk::DescriptorSetLayout CreateEnvironmentLayout()
    {
        DescriptorPool& descriptorPool = *VulkanContext::descriptorPool;

        const DescriptorDescription environmentDescriptorDescription{
            1, vk::DescriptorType::eCombinedImageSampler,
            vk::ShaderStageFlagBits::eCompute,
            vk::DescriptorBindingFlags()
        };

        return descriptorPool.CreateDescriptorSetLayout({ environmentDescriptorDescription });;
    }

    vk::DescriptorSetLayout CreateTargetLayout()
    {
        DescriptorPool& descriptorPool = *VulkanContext::descriptorPool;

        const DescriptorDescription targetDescriptorDescription{
            1, vk::DescriptorType::eStorageImage,
            vk::ShaderStageFlagBits::eCompute,
            vk::DescriptorBindingFlags()
        };

        return descriptorPool.CreateDescriptorSetLayout({ targetDescriptorDescription });;
    }

    std::unique_ptr<ComputePipeline> CreateIrradiancePipeline(
            const std::vector<vk::DescriptorSetLayout>& layouts)
    {
        const std::tuple specializationValues = std::make_tuple(kWorkGroupSize.x, kWorkGroupSize.y, 1);

        const ShaderModule shaderModule = VulkanContext::shaderManager->CreateShaderModule(
                vk::ShaderStageFlagBits::eCompute, kIrradianceShaderPath, specializationValues);

        const vk::PushConstantRange pushConstantRange(vk::ShaderStageFlagBits::eCompute,
                0, sizeof(vk::Extent2D) + sizeof(uint32_t));

        const ComputePipeline::Description pipelineDescription{
            shaderModule, layouts, { pushConstantRange }
        };

        std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(pipelineDescription);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

        return pipeline;
    }

    std::unique_ptr<ComputePipeline> CreateReflectionPipeline(
            const std::vector<vk::DescriptorSetLayout>& layouts)
    {
        const std::tuple specializationValues = std::make_tuple(kWorkGroupSize.x, kWorkGroupSize.y, 1);

        const ShaderModule shaderModule = VulkanContext::shaderManager->CreateShaderModule(
                vk::ShaderStageFlagBits::eCompute, kReflectionShaderPath, specializationValues);

        const vk::PushConstantRange pushConstantRange(vk::ShaderStageFlagBits::eCompute,
                0, sizeof(vk::Extent2D) + sizeof(float) + sizeof(uint32_t));

        const ComputePipeline::Description pipelineDescription{
            shaderModule, layouts, { pushConstantRange }
        };

        std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(pipelineDescription);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

        return pipeline;
    }

    const vk::Extent2D& GetIrradianceExtent(const vk::Extent2D& environmentExtent)
    {
        if (environmentExtent.width <= kMaxIrradianceExtent.width)
        {
            return environmentExtent;
        }

        return kMaxIrradianceExtent;
    }

    const vk::Extent2D& GetReflectionExtent(const vk::Extent2D& environmentExtent)
    {
        if (environmentExtent.width <= kMaxReflectionExtent.width)
        {
            return environmentExtent;
        }

        return kMaxReflectionExtent;
    }

    vk::Image CreateIrradianceImage(vk::Format format, const vk::Extent2D& extent)
    {
        const vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eTransferDst
                | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled;

        const ImageDescription imageDescription{
            ImageType::eCube, format,
            VulkanHelpers::GetExtent3D(extent),
            1, ImageHelpers::kCubeFaceCount,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal, usage,
            vk::ImageLayout::eUndefined,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        return VulkanContext::imageManager->CreateImage(imageDescription, ImageCreateFlags::kNone);
    }

    vk::Image CreateReflectionImage(vk::Format format, const vk::Extent2D& extent)
    {
        const vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eTransferDst
                | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled;

        const ImageDescription imageDescription{
            ImageType::eCube, format,
            VulkanHelpers::GetExtent3D(extent),
            ImageHelpers::CalculateMipLevelCount(extent),
            ImageHelpers::kCubeFaceCount,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal, usage,
            vk::ImageLayout::eUndefined,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        return VulkanContext::imageManager->CreateImage(imageDescription, ImageCreateFlags::kNone);
    }

    vk::DescriptorSet AllocateEnvironmentDescriptorSet(
            vk::DescriptorSetLayout layout, const vk::ImageView environmentView)
    {
        const DescriptorPool& descriptorPool = *VulkanContext::descriptorPool;

        const vk::DescriptorSet descriptorSet = descriptorPool.AllocateDescriptorSets({ layout }).front();

        const DescriptorData descriptorData = DescriptorHelpers::GetData(Renderer::defaultSampler, environmentView);

        descriptorPool.UpdateDescriptorSet(descriptorSet, { descriptorData }, 0);

        return descriptorSet;
    }

    static std::vector<vk::DescriptorSet> AllocateCubeFacesDescriptorSets(
            vk::DescriptorSetLayout layout, const ImageHelpers::CubeFacesViews& cubeFacesViews)
    {
        const std::vector<vk::DescriptorSet> cubeFacesDescriptorSets
                = VulkanContext::descriptorPool->AllocateDescriptorSets(Repeat(layout, cubeFacesViews.size()));

        for (size_t i = 0; i < cubeFacesViews.size(); ++i)
        {
            const DescriptorData descriptorData = DescriptorHelpers::GetData(cubeFacesViews[i]);

            VulkanContext::descriptorPool->UpdateDescriptorSet(cubeFacesDescriptorSets[i], { descriptorData }, 0);
        }

        return cubeFacesDescriptorSets;
    }
}

ImageBasedLighting::ImageBasedLighting()
{
    environmentLayout = Details::CreateEnvironmentLayout();
    targetLayout = Details::CreateTargetLayout();

    irradiancePipeline = Details::CreateIrradiancePipeline({ environmentLayout, targetLayout });
    reflectionPipeline = Details::CreateReflectionPipeline({ environmentLayout, targetLayout });

    samplers = Details::CreateSamplers();
}

ImageBasedLighting::~ImageBasedLighting()
{
    VulkanContext::descriptorPool->DestroyDescriptorSetLayout(environmentLayout);
    VulkanContext::descriptorPool->DestroyDescriptorSetLayout(targetLayout);

    VulkanContext::textureManager->DestroySampler(samplers.specularBRDF);
    VulkanContext::textureManager->DestroySampler(samplers.irradiance);
    VulkanContext::textureManager->DestroySampler(samplers.reflection);
}

ImageBasedLighting::Textures ImageBasedLighting::GenerateTextures(const Texture& environmentTexture) const
{
    const ImageDescription& environmentDescription
            = VulkanContext::imageManager->GetImageDescription(environmentTexture.image);

    const vk::Extent2D environmentExtent = VulkanHelpers::GetExtent2D(environmentDescription.extent);
    const vk::Extent2D irradianceExtent = Details::GetIrradianceExtent(environmentExtent);
    const vk::Extent2D reflectionExtent = Details::GetReflectionExtent(environmentExtent);

    const vk::Image irradianceImage = Details::CreateIrradianceImage(environmentDescription.format, irradianceExtent);
    const vk::Image reflectionImage = Details::CreateReflectionImage(environmentDescription.format, reflectionExtent);

    const ImageHelpers::CubeFacesViews irradianceFacesViews = ImageHelpers::CreateCubeFacesViews(irradianceImage, 0);

    const uint32_t reflectionMipLevelCount = ImageHelpers::CalculateMipLevelCount(reflectionExtent);
    std::vector<ImageHelpers::CubeFacesViews> reflectionMipLevelsFacesViews(reflectionMipLevelCount);
    for (uint32_t i = 0; i < reflectionMipLevelsFacesViews.size(); ++i)
    {
        reflectionMipLevelsFacesViews[i] = ImageHelpers::CreateCubeFacesViews(reflectionImage, i);
    }

    const vk::DescriptorSet environmentDescriptorSet
            = Details::AllocateEnvironmentDescriptorSet(environmentLayout, environmentTexture.view);

    const std::vector<vk::DescriptorSet> irradianceFacesDescriptorSets
            = Details::AllocateCubeFacesDescriptorSets(targetLayout, irradianceFacesViews);

    std::vector<std::vector<vk::DescriptorSet>> reflectionMipLevelsFacesDescriptorSets(reflectionMipLevelCount);
    for (uint32_t i = 0; i < reflectionMipLevelsFacesViews.size(); ++i)
    {
        reflectionMipLevelsFacesDescriptorSets[i]
                = Details::AllocateCubeFacesDescriptorSets(targetLayout, reflectionMipLevelsFacesViews[i]);
    }

    const vk::ImageSubresourceRange reflectionSubresourceRange(
            vk::ImageAspectFlagBits::eColor,
            0, reflectionMipLevelCount,
            0, ImageHelpers::kCubeFaceCount);

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

                    ImageHelpers::TransitImageLayout(commandBuffer, irradianceImage,
                            ImageHelpers::kCubeColor, layoutTransition);

                    ImageHelpers::TransitImageLayout(commandBuffer, reflectionImage,
                            reflectionSubresourceRange, layoutTransition);
                }

                {
                    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, irradiancePipeline->Get());

                    commandBuffer.pushConstants<vk::Extent2D>(irradiancePipeline->GetLayout(),
                            vk::ShaderStageFlagBits::eCompute, 0, { irradianceExtent });

                    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                            irradiancePipeline->GetLayout(), 0, { environmentDescriptorSet }, {});

                    const glm::uvec3 groupCount = ComputeHelpers::CalculateWorkGroupCount(
                            irradianceExtent, Details::kWorkGroupSize);

                    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                            irradiancePipeline->GetLayout(), 1, { irradianceFacesDescriptorSets[faceIndex] }, {});

                    commandBuffer.pushConstants<uint32_t>(irradiancePipeline->GetLayout(),
                            vk::ShaderStageFlagBits::eCompute, sizeof(vk::Extent2D), { faceIndex });

                    commandBuffer.dispatch(groupCount.x, groupCount.y, groupCount.z);
                }

                {
                    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, reflectionPipeline->Get());

                    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                            reflectionPipeline->GetLayout(), 0, { environmentDescriptorSet }, {});

                    const glm::uvec3 groupCount = ComputeHelpers::CalculateWorkGroupCount(
                            reflectionExtent, Details::kWorkGroupSize);

                    for (uint32_t mipLevel = 0; mipLevel < reflectionMipLevelCount; ++mipLevel)
                    {
                        const vk::Extent2D mipLevelExtent{
                            std::max(static_cast<uint32_t>(reflectionExtent.width * std::pow(0.5f, mipLevel)), 1u),
                            std::max(static_cast<uint32_t>(reflectionExtent.height * std::pow(0.5f, mipLevel)), 1u),
                        };

                        const float roughness = static_cast<float>(mipLevel) / (reflectionMipLevelCount - 1);

                        commandBuffer.pushConstants<vk::Extent2D>(reflectionPipeline->GetLayout(),
                                vk::ShaderStageFlagBits::eCompute, 0, { mipLevelExtent });

                        commandBuffer.pushConstants<float>(reflectionPipeline->GetLayout(),
                                vk::ShaderStageFlagBits::eCompute, sizeof(vk::Extent2D), { roughness });

                        const vk::DescriptorSet reflectionFaceDescriptorSet
                                = reflectionMipLevelsFacesDescriptorSets[mipLevel][faceIndex];

                        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                                reflectionPipeline->GetLayout(), 1, { reflectionFaceDescriptorSet }, {});

                        const uint32_t pushConstantOffset = sizeof(vk::Extent2D) + sizeof(uint32_t);

                        commandBuffer.pushConstants<uint32_t>(reflectionPipeline->GetLayout(),
                                vk::ShaderStageFlagBits::eCompute, pushConstantOffset, { faceIndex });

                        commandBuffer.dispatch(groupCount.x, groupCount.y, groupCount.z);
                    }
                }

                if (faceIndex == ImageHelpers::kCubeFaceCount - 1)
                {
                    const ImageLayoutTransition layoutTransition{
                        vk::ImageLayout::eGeneral,
                        vk::ImageLayout::eShaderReadOnlyOptimal,
                        PipelineBarrier{
                            SyncScope::kComputeShaderWrite,
                            SyncScope::kShaderRead
                        }
                    };

                    ImageHelpers::TransitImageLayout(commandBuffer, irradianceImage,
                            ImageHelpers::kCubeColor, layoutTransition);


                    ImageHelpers::TransitImageLayout(commandBuffer, reflectionImage,
                            reflectionSubresourceRange, layoutTransition);
                }
            });
    }

    VulkanContext::descriptorPool->FreeDescriptorSets({ environmentDescriptorSet });
    VulkanContext::descriptorPool->FreeDescriptorSets(irradianceFacesDescriptorSets);
    for (const auto& reflectionFacesDescriptorSets : reflectionMipLevelsFacesDescriptorSets)
    {
        VulkanContext::descriptorPool->FreeDescriptorSets(reflectionFacesDescriptorSets);
    }

    for (const auto& view : irradianceFacesViews)
    {
        VulkanContext::imageManager->DestroyImageView(irradianceImage, view);
    }

    for (const auto& reflectionFacesViews : reflectionMipLevelsFacesViews)
    {
        for (const auto& view : reflectionFacesViews)
        {
            VulkanContext::imageManager->DestroyImageView(reflectionImage, view);
        }
    }

    const vk::ImageView irradianceView = VulkanContext::imageManager->CreateView(
            irradianceImage, vk::ImageViewType::eCube, ImageHelpers::kCubeColor);

    const vk::ImageView reflectionView = VulkanContext::imageManager->CreateView(
            reflectionImage, vk::ImageViewType::eCube, reflectionSubresourceRange);

    const Texture irradianceTexture{ irradianceImage, irradianceView };
    const Texture reflectionTexture{ reflectionImage, reflectionView };

    return Textures{ irradianceTexture, reflectionTexture };
}

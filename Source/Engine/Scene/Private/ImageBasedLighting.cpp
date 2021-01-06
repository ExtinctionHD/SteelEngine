#include "Engine/Scene/ImageBasedLighting.hpp"

#include "Engine/Filesystem/Filepath.hpp"
#include "Engine/Render/Vulkan/ComputeHelpers.hpp"
#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/ComputePipeline.hpp"

namespace Details
{
    constexpr glm::uvec2 kWorkGroupSize(16, 16);

    constexpr vk::Extent2D kMaxIrradianceExtent(512, 512);

    const Filepath kIrradianceShaderPath("~/Shaders/Compute/ImageBasedLighting/Irradiance.comp");

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

    std::unique_ptr<ComputePipeline> CreateIrradiancePipeline(const std::vector<vk::DescriptorSetLayout>& layouts)
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

    const vk::Extent2D& GetIrradianceExtent(const vk::Extent2D& environmentExtent)
    {
        if (environmentExtent.width <= kMaxIrradianceExtent.width)
        {
            return environmentExtent;
        }

        return kMaxIrradianceExtent;
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

    vk::DescriptorSet AllocateEnvironmentDescriptorSet(vk::DescriptorSetLayout layout,
            const vk::ImageView environmentView, vk::Sampler environmentSampler)
    {
        const DescriptorPool& descriptorPool = *VulkanContext::descriptorPool;

        const vk::DescriptorSet descriptorSet = descriptorPool.AllocateDescriptorSets({ layout }).front();

        const DescriptorData descriptorData = DescriptorHelpers::GetData(environmentSampler, environmentView);

        descriptorPool.UpdateDescriptorSet(descriptorSet, { descriptorData }, 0);

        return descriptorSet;
    }

    static std::vector<vk::DescriptorSet> AllocateCubeFacesDescriptorSets(vk::DescriptorSetLayout layout,
            const ImageHelpers::CubeFacesViews& cubeFacesViews)
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
}

ImageBasedLighting::~ImageBasedLighting()
{
    VulkanContext::descriptorPool->DestroyDescriptorSetLayout(environmentLayout);
    VulkanContext::descriptorPool->DestroyDescriptorSetLayout(targetLayout);
}

Texture ImageBasedLighting::GenerateIrradianceTexture(
        const Texture& environmentTexture, vk::Sampler environmentSampler) const
{
    ImageManager& imageManager = *VulkanContext::imageManager;

    const ImageDescription& environmentDescription
            = imageManager.GetImageDescription(environmentTexture.image);

    const vk::Extent2D irradianceExtent = Details::GetIrradianceExtent(
            VulkanHelpers::GetExtent2D(environmentDescription.extent));

    const vk::Image irradianceImage = Details::CreateIrradianceImage(
            environmentDescription.format, irradianceExtent);

    const ImageHelpers::CubeFacesViews irradianceFacesViews = ImageHelpers::CreateCubeFacesViews(irradianceImage);

    const vk::DescriptorSet environmentDescriptorSet
            = Details::AllocateEnvironmentDescriptorSet(environmentLayout, environmentTexture.view, environmentSampler);

    const std::vector<vk::DescriptorSet> irradianceFacesDescriptorSets
            = Details::AllocateCubeFacesDescriptorSets(targetLayout, irradianceFacesViews);

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

                ImageHelpers::TransitImageLayout(commandBuffer, irradianceImage,
                        ImageHelpers::kCubeColor, layoutTransition);
            }

            commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, irradiancePipeline->Get());

            commandBuffer.pushConstants<vk::Extent2D>(irradiancePipeline->GetLayout(),
                    vk::ShaderStageFlagBits::eCompute, 0, { irradianceExtent });

            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                    irradiancePipeline->GetLayout(), 0, { environmentDescriptorSet }, {});

            const glm::uvec3 groupCount = ComputeHelpers::CalculateWorkGroupCount(
                    irradianceExtent, Details::kWorkGroupSize);

            for (uint32_t i = 0; i < ImageHelpers::kCubeFaceCount; ++i)
            {
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                        irradiancePipeline->GetLayout(), 1, { irradianceFacesDescriptorSets[i] }, {});

                commandBuffer.pushConstants<uint32_t>(irradiancePipeline->GetLayout(),
                        vk::ShaderStageFlagBits::eCompute, sizeof(vk::Extent2D), { i });

                commandBuffer.dispatch(groupCount.x, groupCount.y, groupCount.z);
            }

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
            }
        });


    VulkanContext::descriptorPool->FreeDescriptorSets({ environmentDescriptorSet });
    VulkanContext::descriptorPool->FreeDescriptorSets(irradianceFacesDescriptorSets);

    for (const auto& view : irradianceFacesViews)
    {
        VulkanContext::imageManager->DestroyImageView(irradianceImage, view);
    }

    const vk::ImageView irradianceView = VulkanContext::imageManager->CreateView(
            irradianceImage, vk::ImageViewType::eCube, ImageHelpers::kCubeColor);

    return Texture{ irradianceImage, irradianceView };
}

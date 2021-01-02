#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"

#include "Engine/Render/Vulkan/ComputePipeline.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"

namespace Details
{
    constexpr glm::uvec2 kWorkGroupSize(16, 16);

    const Filepath kShaderPath("~/Shaders/Compute/PanoramaToCube.comp");

    vk::DescriptorSetLayout CreatePanoramaLayout()
    {
        const DescriptorDescription descriptorDescription{
            1, vk::DescriptorType::eCombinedImageSampler,
            vk::ShaderStageFlagBits::eCompute,
            vk::DescriptorBindingFlags()
        };

        return VulkanContext::descriptorPool->CreateDescriptorSetLayout({ descriptorDescription });
    }

    vk::DescriptorSetLayout CreateCubeFaceLayout()
    {
        const DescriptorDescription descriptorDescription{
            1, vk::DescriptorType::eStorageImage,
            vk::ShaderStageFlagBits::eCompute,
            vk::DescriptorBindingFlags()
        };

        return VulkanContext::descriptorPool->CreateDescriptorSetLayout({ descriptorDescription });
    }

    std::unique_ptr<ComputePipeline> CreatePanoramaToCubePipeline(
            const std::vector<vk::DescriptorSetLayout>& layouts)
    {
        const std::tuple specializationValues = std::make_tuple(kWorkGroupSize.x, kWorkGroupSize.y, 1);

        const ShaderModule shaderModule = VulkanContext::shaderManager->CreateShaderModule(
                vk::ShaderStageFlagBits::eCompute, kShaderPath, specializationValues);

        const vk::PushConstantRange pushConstantRange(vk::ShaderStageFlagBits::eCompute,
                0, sizeof(vk::Extent2D) + sizeof(uint32_t));

        const ComputePipeline::Description pipelineDescription{
            shaderModule, layouts, { pushConstantRange }
        };

        std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(pipelineDescription);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

        return pipeline;
    }

    vk::DescriptorSet AllocatePanoramaDescriptorSet(vk::DescriptorSetLayout layout,
            const Texture& panoramaTexture, vk::Sampler panoramaSampler)
    {
        const DescriptorPool& descriptorPool = *VulkanContext::descriptorPool;

        const vk::DescriptorSet descriptorSet = descriptorPool.AllocateDescriptorSets({ layout }).front();

        const DescriptorData descriptorData = DescriptorHelpers::GetData(panoramaSampler, panoramaTexture.view);

        descriptorPool.UpdateDescriptorSet(descriptorSet, { descriptorData }, 0);

        return descriptorSet;
    }

    std::vector<vk::DescriptorSet> AllocateCubeFacesDescriptorSets(
            vk::DescriptorSetLayout layout, vk::Image cubeImage)
    {
        const DescriptorPool& descriptorPool = *VulkanContext::descriptorPool;

        std::vector<vk::DescriptorSet> descriptorSets(TextureHelpers::kCubeFaceCount);
        for (uint32_t i = 0; i < TextureHelpers::kCubeFaceCount; ++i)
        {
            descriptorSets[i] = descriptorPool.AllocateDescriptorSets({ layout }).front();

            const vk::ImageSubresourceRange subresourceRange(
                    vk::ImageAspectFlagBits::eColor, 0, 1, i, 1);

            const vk::ImageView view = VulkanContext::imageManager->CreateView(cubeImage,
                    vk::ImageViewType::e2D, subresourceRange);

            const DescriptorData descriptorData = DescriptorHelpers::GetData(view);

            descriptorPool.UpdateDescriptorSet(descriptorSets[i], { descriptorData }, 0);
        }

        return descriptorSets;
    }

    glm::uvec3 CalculateWorkGroupCount(const vk::Extent2D& extent)
    {
        const auto calculate = [](uint32_t dimension, uint32_t groupSize)
            {
                return dimension / groupSize + std::min(dimension % groupSize, 1u);
            };

        glm::uvec3 groupCount;

        groupCount.x = calculate(extent.width, kWorkGroupSize.x);
        groupCount.y = calculate(extent.height, kWorkGroupSize.y);
        groupCount.z = 1;

        return groupCount;
    }
}

PanoramaToCube::PanoramaToCube()
{
    panoramaLayout = Details::CreatePanoramaLayout();
    cubeFaceLayout = Details::CreateCubeFaceLayout();

    pipeline = Details::CreatePanoramaToCubePipeline({ panoramaLayout, cubeFaceLayout });
}

PanoramaToCube::~PanoramaToCube()
{
    VulkanContext::descriptorPool->DestroyDescriptorSetLayout(panoramaLayout);
    VulkanContext::descriptorPool->DestroyDescriptorSetLayout(cubeFaceLayout);
}

void PanoramaToCube::Convert(const Texture& panoramaTexture, vk::Sampler panoramaSampler,
        vk::Image cubeImage, const vk::Extent2D& cubeImageExtent) const
{
    const vk::DescriptorSet panoramaDescriptorSet
            = Details::AllocatePanoramaDescriptorSet(panoramaLayout, panoramaTexture, panoramaSampler);

    const std::vector<vk::DescriptorSet> cubeFacesDescriptorSets
            = Details::AllocateCubeFacesDescriptorSets(cubeFaceLayout, cubeImage);

    VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
        {
            const vk::ImageSubresourceRange subresourceRange(
                    vk::ImageAspectFlagBits::eColor, 0, 1, 0, TextureHelpers::kCubeFaceCount);

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
                    subresourceRange, layoutTransition);
            }

            commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline->Get());

            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                    pipeline->GetLayout(), 0, { panoramaDescriptorSet }, {});

            commandBuffer.pushConstants(pipeline->GetLayout(),
                    vk::ShaderStageFlagBits::eCompute, 0, vk::ArrayProxy<const vk::Extent2D>{ cubeImageExtent });

            const glm::uvec3 groupCount = Details::CalculateWorkGroupCount(cubeImageExtent);

            for (uint32_t i = 0; i < TextureHelpers::kCubeFaceCount; ++i)
            {
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                        pipeline->GetLayout(), 1, { cubeFacesDescriptorSets[i] }, {});

                commandBuffer.pushConstants(pipeline->GetLayout(),
                        vk::ShaderStageFlagBits::eCompute, sizeof(vk::Extent2D), sizeof(uint32_t), &i);

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

                ImageHelpers::TransitImageLayout(commandBuffer, cubeImage,
                    subresourceRange, layoutTransition);
            }
        });

    VulkanContext::descriptorPool->FreeDescriptorSets({ panoramaDescriptorSet });
    VulkanContext::descriptorPool->FreeDescriptorSets(cubeFacesDescriptorSets);
}

#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"

#include "Engine/Render/Vulkan/ComputeHelpers.hpp"
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
            const vk::ImageView panoramaView, vk::Sampler panoramaSampler)
    {
        const DescriptorPool& descriptorPool = *VulkanContext::descriptorPool;

        const vk::DescriptorSet descriptorSet = descriptorPool.AllocateDescriptorSets({ layout }).front();

        const DescriptorData descriptorData = DescriptorHelpers::GetData(panoramaSampler, panoramaView);

        descriptorPool.UpdateDescriptorSet(descriptorSet, { descriptorData }, 0);

        return descriptorSet;
    }

    std::vector<vk::DescriptorSet> AllocateCubeFacesDescriptorSets(vk::DescriptorSetLayout layout,
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
    const ImageHelpers::CubeFacesViews cubeFacesViews = ImageHelpers::CreateCubeFacesViews(cubeImage);

    const vk::DescriptorSet panoramaDescriptorSet
            = Details::AllocatePanoramaDescriptorSet(panoramaLayout, panoramaTexture.view, panoramaSampler);

    const std::vector<vk::DescriptorSet> cubeFacesDescriptorSets
            = Details::AllocateCubeFacesDescriptorSets(cubeFaceLayout, cubeFacesViews);

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

            commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline->Get());

            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                    pipeline->GetLayout(), 0, { panoramaDescriptorSet }, {});

            commandBuffer.pushConstants<vk::Extent2D>(pipeline->GetLayout(),
                    vk::ShaderStageFlagBits::eCompute, 0, { cubeImageExtent });

            const glm::uvec3 groupCount = ComputeHelpers::CalculateWorkGroupCount(
                    cubeImageExtent, Details::kWorkGroupSize);

            for (uint32_t i = 0; i < ImageHelpers::kCubeFaceCount; ++i)
            {
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                        pipeline->GetLayout(), 1, { cubeFacesDescriptorSets[i] }, {});

                commandBuffer.pushConstants<uint32_t>(pipeline->GetLayout(),
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

                ImageHelpers::TransitImageLayout(commandBuffer, cubeImage,
                        ImageHelpers::kCubeColor, layoutTransition);
            }
        });

    VulkanContext::descriptorPool->FreeDescriptorSets({ panoramaDescriptorSet });
    VulkanContext::descriptorPool->FreeDescriptorSets(cubeFacesDescriptorSets);

    for (const auto& view : cubeFacesViews)
    {
        VulkanContext::imageManager->DestroyImageView(cubeImage, view);
    }
}

#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"

#include "Engine/Render/Renderer.hpp"
#include "Engine/Render/Vulkan/ComputeHelpers.hpp"
#include "Engine/Render/Vulkan/ComputePipeline.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"

namespace Details
{
    static constexpr glm::uvec2 kWorkGroupSize(16, 16);

    static const Filepath kShaderPath("~/Shaders/Compute/PanoramaToCube.comp");

    static vk::DescriptorSetLayout CreatePanoramaLayout()
    {
        const DescriptorDescription descriptorDescription{
            1, vk::DescriptorType::eCombinedImageSampler,
            vk::ShaderStageFlagBits::eCompute,
            vk::DescriptorBindingFlags()
        };

        return VulkanContext::descriptorPool->CreateDescriptorSetLayout({ descriptorDescription });
    }

    static vk::DescriptorSetLayout CreateCubeFaceLayout()
    {
        const DescriptorDescription descriptorDescription{
            1, vk::DescriptorType::eStorageImage,
            vk::ShaderStageFlagBits::eCompute,
            vk::DescriptorBindingFlags()
        };

        return VulkanContext::descriptorPool->CreateDescriptorSetLayout({ descriptorDescription });
    }

    static std::unique_ptr<ComputePipeline> CreatePanoramaToCubePipeline(
            const std::vector<vk::DescriptorSetLayout>& layouts)
    {
        const std::tuple specializationValues = std::make_tuple(kWorkGroupSize.x, kWorkGroupSize.y, 1);

        const ShaderModule shaderModule = VulkanContext::shaderManager->CreateShaderModule(
                vk::ShaderStageFlagBits::eCompute, kShaderPath, specializationValues);

        const vk::PushConstantRange pushConstantRange(
                vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint32_t));

        const ComputePipeline::Description pipelineDescription{
            shaderModule, layouts, { pushConstantRange }
        };

        std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(pipelineDescription);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

        return pipeline;
    }

    static vk::DescriptorSet AllocatePanoramaDescriptorSet(
            vk::DescriptorSetLayout layout, const vk::ImageView panoramaView)
    {
        const DescriptorPool& descriptorPool = *VulkanContext::descriptorPool;

        const vk::DescriptorSet descriptorSet = descriptorPool.AllocateDescriptorSets({ layout }).front();

        const DescriptorData descriptorData = DescriptorHelpers::GetData(Renderer::defaultSampler, panoramaView);

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

void PanoramaToCube::Convert(const Texture& panoramaTexture,
        vk::Image cubeImage, const vk::Extent2D& cubeImageExtent) const
{
    const ImageHelpers::CubeFacesViews cubeFacesViews = ImageHelpers::CreateCubeFacesViews(cubeImage, 0);

    const vk::DescriptorSet panoramaDescriptorSet
            = Details::AllocatePanoramaDescriptorSet(panoramaLayout, panoramaTexture.view);

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

            const glm::uvec3 groupCount = ComputeHelpers::CalculateWorkGroupCount(
                    cubeImageExtent, Details::kWorkGroupSize);

            for (uint32_t faceIndex = 0; faceIndex < ImageHelpers::kCubeFaceCount; ++faceIndex)
            {
                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                        pipeline->GetLayout(), 1, { cubeFacesDescriptorSets[faceIndex] }, {});

                commandBuffer.pushConstants<uint32_t>(pipeline->GetLayout(),
                        vk::ShaderStageFlagBits::eCompute, 0, { faceIndex });

                commandBuffer.dispatch(groupCount.x, groupCount.y, groupCount.z);
            }
        });

    VulkanContext::descriptorPool->FreeDescriptorSets({ panoramaDescriptorSet });
    VulkanContext::descriptorPool->FreeDescriptorSets(cubeFacesDescriptorSets);

    for (const auto& view : cubeFacesViews)
    {
        VulkanContext::imageManager->DestroyImageView(cubeImage, view);
    }
}

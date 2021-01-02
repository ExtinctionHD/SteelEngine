#include "Engine/Scene/Environment.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/ComputePipeline.hpp"

namespace Details
{
    struct LuminanceData
    {
        Texture texture;
        DescriptorSet descriptorSet;
    };

    struct LocationData
    {
        vk::Buffer buffer;
        DescriptorSet descriptorSet;
    };

    constexpr glm::uvec2 kLuminanceBlockSize(8, 8);

    const Filepath kLuminanceShaderPath("~/Shaders/Compute/DirectLight/Luminance.comp");
    const Filepath kLocationShaderPath("~/Shaders/Compute/DirectLight/Location.comp");

    std::unique_ptr<ComputePipeline> CreateLuminancePipeline(const vk::Extent2D& panoramaExtent,
            const std::vector<vk::DescriptorSetLayout> layouts)
    {
        Assert(panoramaExtent.width % kLuminanceBlockSize.x == 0);
        Assert(panoramaExtent.height % kLuminanceBlockSize.y == 0);

        const std::tuple specializationValues = std::make_tuple(kLuminanceBlockSize.x, kLuminanceBlockSize.y, 1);

        const ShaderModule shaderModule = VulkanContext::shaderManager->CreateShaderModule(
                vk::ShaderStageFlagBits::eCompute, kLuminanceShaderPath, specializationValues);

        const ComputePipeline::Description pipelineDescription{
            shaderModule, layouts, {}
        };

        std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(pipelineDescription);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

        return pipeline;
    }

    std::unique_ptr<ComputePipeline> CreateLocationPipeline(const vk::Extent2D& panoramaExtent,
            const std::vector<vk::DescriptorSetLayout> layouts)
    {
        const uint32_t maxWorkGroupInvocations = VulkanContext::device->GetLimits().maxComputeWorkGroupInvocations;

        glm::uvec2 workGroupSize(
                panoramaExtent.width / kLuminanceBlockSize.x,
                panoramaExtent.height / kLuminanceBlockSize.y);

        glm::uvec2 loadCount(1, 1);

        while (workGroupSize.x * workGroupSize.y > maxWorkGroupInvocations)
        {
            if (workGroupSize.x > workGroupSize.y)
            {
                workGroupSize.x = workGroupSize.x / 2 + workGroupSize.x % 2;
                loadCount.x *= 2;
            }
            else
            {
                workGroupSize.y = workGroupSize.y / 2 + workGroupSize.y % 2;
                loadCount.y *= 2;
            }
        }

        const std::tuple specializationValues = std::make_tuple(
                workGroupSize.x, workGroupSize.y, 1, loadCount.x, loadCount.y);

        const ShaderModule shaderModule = VulkanContext::shaderManager->CreateShaderModule(
                vk::ShaderStageFlagBits::eCompute, kLocationShaderPath, specializationValues);

        const ComputePipeline::Description pipelineDescription{
            shaderModule, layouts, {}
        };

        std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(pipelineDescription);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

        return pipeline;
    }

    DescriptorSet CreatePanoramaDescriptorSet(vk::ImageView panoramaView)
    {
        const DescriptorDescription descriptorDescription{
            1, vk::DescriptorType::eStorageImage,
            vk::ShaderStageFlagBits::eCompute,
            vk::DescriptorBindingFlags()
        };

        const DescriptorData descriptorData = DescriptorHelpers::GetData(panoramaView);

        return DescriptorHelpers::CreateDescriptorSet({ descriptorDescription }, { descriptorData });
    }

    LuminanceData CreateLuminanceData(const vk::Extent2D& panoramaExtent)
    {
        const vk::Extent2D extent(
                panoramaExtent.width / kLuminanceBlockSize.x,
                panoramaExtent.height / kLuminanceBlockSize.y);

        const ImageDescription imageDescription{
            ImageType::e2D, vk::Format::eR32Uint,
            VulkanHelpers::GetExtent3D(extent),
            1, 1, vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eStorage,
            vk::ImageLayout::eUndefined,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        const vk::Image image = VulkanContext::imageManager->CreateImage(
                imageDescription, ImageCreateFlags::kNone);

        const vk::ImageView view = VulkanContext::imageManager->CreateView(
                image, vk::ImageViewType::e2D, ImageHelpers::kFlatColor);

        const DescriptorDescription descriptorDescription{
            1, vk::DescriptorType::eStorageImage,
            vk::ShaderStageFlagBits::eCompute,
            vk::DescriptorBindingFlags()
        };

        const DescriptorData descriptorData = DescriptorHelpers::GetData(view);

        const DescriptorSet descriptorSet = DescriptorHelpers::CreateDescriptorSet(
                { descriptorDescription }, { descriptorData });

        return LuminanceData{ Texture{ image, view }, descriptorSet };
    }

    LocationData CreateLocationData()
    {
        const vk::MemoryPropertyFlags memoryProperties
                = vk::MemoryPropertyFlagBits::eDeviceLocal
                | vk::MemoryPropertyFlagBits::eHostVisible
                | vk::MemoryPropertyFlagBits::eHostCoherent;

        const BufferDescription bufferDescription{
            sizeof(glm::uvec2),
            vk::BufferUsageFlagBits::eStorageBuffer,
            memoryProperties
        };

        const vk::Buffer buffer = VulkanContext::bufferManager->CreateBuffer(
                bufferDescription, BufferCreateFlags::kNone);

        const DescriptorDescription descriptorDescription{
            1, vk::DescriptorType::eStorageBuffer,
            vk::ShaderStageFlagBits::eCompute,
            vk::DescriptorBindingFlags()
        };

        const BufferInfo bufferInfo{ vk::DescriptorBufferInfo(buffer, 0, VK_WHOLE_SIZE) };

        const DescriptorData descriptorData{
            vk::DescriptorType::eStorageBuffer,
            bufferInfo
        };

        const DescriptorSet descriptorSet = DescriptorHelpers::CreateDescriptorSet(
                { descriptorDescription }, { descriptorData });

        return LocationData{ buffer, descriptorSet };
    }

    glm::vec2 RetrieveLocation(const LocationData& locationData)
    {
        const MemoryBlock memoryBlock = VulkanContext::memoryManager->GetBufferMemoryBlock(locationData.buffer);

        const ByteAccess locationBytesAccess = VulkanContext::memoryManager->MapMemory(memoryBlock);

        return *reinterpret_cast<const glm::uvec2*>(locationBytesAccess.data);
    }

    glm::vec4 CalculateLightDirection(const glm::uvec2&, const vk::Extent2D&)
    {
        return {};
    }

    glm::vec4 RetrieveLightDirection(const Texture& panoramaTexture, const vk::Extent2D& panoramaExtent)
    {
        const vk::ImageView panoramaView = VulkanContext::imageManager->CreateView(
                panoramaTexture.image, vk::ImageViewType::e2D, ImageHelpers::kFlatColor);

        const DescriptorSet panoramaDescriptorSet = CreatePanoramaDescriptorSet(panoramaView);

        const LuminanceData luminanceData = CreateLuminanceData(panoramaExtent);
        const LocationData locationData = CreateLocationData();

        std::unique_ptr<ComputePipeline> luminancePipeline = CreateLuminancePipeline(panoramaExtent,
                { panoramaDescriptorSet.layout, luminanceData.descriptorSet.layout });

        std::unique_ptr<ComputePipeline> locationPipeline = CreateLocationPipeline(panoramaExtent,
                { luminanceData.descriptorSet.layout, locationData.descriptorSet.layout });

        VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
            {
                {
                    const ImageLayoutTransition layoutTransition{
                        vk::ImageLayout::eShaderReadOnlyOptimal,
                        vk::ImageLayout::eGeneral,
                        PipelineBarrier{
                            SyncScope::kWaitForNone,
                            SyncScope::kComputeShaderRead
                        }
                    };

                    ImageHelpers::TransitImageLayout(commandBuffer, panoramaTexture.image,
                            ImageHelpers::kFlatColor, layoutTransition);
                }

                {
                    const ImageLayoutTransition layoutTransition{
                        vk::ImageLayout::eUndefined,
                        vk::ImageLayout::eGeneral,
                        PipelineBarrier{
                            SyncScope::kWaitForNone,
                            SyncScope::kComputeShaderWrite
                        }
                    };

                    ImageHelpers::TransitImageLayout(commandBuffer, luminanceData.texture.image,
                            ImageHelpers::kFlatColor, layoutTransition);
                }

                const std::vector<vk::DescriptorSet> luminanceDescriptorSets{
                    panoramaDescriptorSet.value, luminanceData.descriptorSet.value
                };

                commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, luminancePipeline->Get());

                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                        luminancePipeline->GetLayout(), 0, luminanceDescriptorSets, {});

                const glm::uvec2 luminanceGroupCount = glm::uvec2(
                        panoramaExtent.width / kLuminanceBlockSize.x,
                        panoramaExtent.height / kLuminanceBlockSize.y);

                commandBuffer.dispatch(luminanceGroupCount.x, luminanceGroupCount.y, 1);

                {
                    const ImageLayoutTransition layoutTransition{
                        vk::ImageLayout::eGeneral,
                        vk::ImageLayout::eGeneral,
                        PipelineBarrier{
                            SyncScope::kComputeShaderWrite,
                            SyncScope::kComputeShaderRead
                        }
                    };

                    ImageHelpers::TransitImageLayout(commandBuffer, luminanceData.texture.image,
                            ImageHelpers::kFlatColor, layoutTransition);
                }

                const std::vector<vk::DescriptorSet> locationDescriptorSets{
                    luminanceData.descriptorSet.value, locationData.descriptorSet.value
                };

                commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, locationPipeline->Get());

                commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                        locationPipeline->GetLayout(), 0, locationDescriptorSets, {});

                commandBuffer.dispatch(1, 1, 1);

                {
                    const ImageLayoutTransition layoutTransition{
                        vk::ImageLayout::eGeneral,
                        vk::ImageLayout::eShaderReadOnlyOptimal,
                        PipelineBarrier{
                            SyncScope::kComputeShaderRead,
                            SyncScope::kBlockNone
                        }
                    };

                    ImageHelpers::TransitImageLayout(commandBuffer, panoramaTexture.image,
                            ImageHelpers::kFlatColor, layoutTransition);
                }
            });

        const glm::uvec2 location = RetrieveLocation(locationData);

        DescriptorHelpers::DestroyDescriptorSet(panoramaDescriptorSet);
        DescriptorHelpers::DestroyDescriptorSet(luminanceData.descriptorSet);
        DescriptorHelpers::DestroyDescriptorSet(locationData.descriptorSet);

        VulkanContext::textureManager->DestroyTexture(luminanceData.texture);
        VulkanContext::bufferManager->DestroyBuffer(locationData.buffer);

        return CalculateLightDirection(location, panoramaExtent);
    }
}

Environment::Environment(const Filepath& path)
{
    const Texture panoramaTexture = VulkanContext::textureManager->CreateTexture(path);

    const vk::Extent2D& panoramaExtent = VulkanHelpers::GetExtent2D(
            VulkanContext::imageManager->GetImageDescription(panoramaTexture.image).extent);

    const vk::Extent2D environmentExtent = vk::Extent2D(panoramaExtent.height / 2, panoramaExtent.height / 2);

    texture = VulkanContext::textureManager->CreateCubeTexture(panoramaTexture, environmentExtent);
    lightDirection = Details::RetrieveLightDirection(panoramaTexture, panoramaExtent);

    VulkanContext::textureManager->DestroyTexture(panoramaTexture);
}

Environment::~Environment()
{
    VulkanContext::textureManager->DestroyTexture(texture);
}

#include "Engine/Scene/DirectLighting.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/ComputePipeline.hpp"

namespace Details
{
    struct LuminanceData
    {
        Texture texture;
        vk::DescriptorSet descriptorSet;
    };

    struct LocationData
    {
        vk::Buffer buffer;
        vk::DescriptorSet descriptorSet;
    };

    struct ParametersData
    {
        vk::Buffer buffer;
        vk::DescriptorSet descriptorSet;
    };

    constexpr glm::uvec2 kLuminanceBlockSize(8, 8);
    constexpr glm::uvec2 kMaxLoadCount(32, 32);

    const Filepath kLuminanceShaderPath("~/Shaders/Compute/DirectLighting/Luminance.comp");
    const Filepath kLocationShaderPath("~/Shaders/Compute/DirectLighting/Location.comp");
    const Filepath kParametersShaderPath("~/Shaders/Compute/DirectLighting/Parameters.comp");

    glm::uvec2 CalculateMaxWorkGroupSize()
    {
        const uint32_t maxWorkGroupInvocations = VulkanContext::device->GetLimits().maxComputeWorkGroupInvocations;

        glm::uvec2 workGroupSize(2, 1);

        while (workGroupSize.x * workGroupSize.y < maxWorkGroupInvocations)
        {
            workGroupSize.x += 2;
            workGroupSize.y += 1;
        }

        workGroupSize.x -= 2;
        workGroupSize.y -= 1;

        return workGroupSize;
    }

    glm::uvec2 CalculateLoadCount(const glm::uvec2& luminanceGroupCount)
    {
        const glm::uvec2 workGroupSize = Details::CalculateMaxWorkGroupSize();

        const glm::uvec2 loadCount = luminanceGroupCount / workGroupSize
                + glm::min(luminanceGroupCount % workGroupSize, 1u);

        Assert(loadCount.x <= kMaxLoadCount.x);
        Assert(loadCount.y <= kMaxLoadCount.y);

        return loadCount;
    }

    std::unique_ptr<ComputePipeline> CreateLuminancePipeline(const std::vector<vk::DescriptorSetLayout> layouts)
    {
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

    std::unique_ptr<ComputePipeline> CreateLocationPipeline(const std::vector<vk::DescriptorSetLayout> layouts)
    {
        const glm::uvec2 workGroupSize = CalculateMaxWorkGroupSize();

        const std::tuple specializationValues = std::make_tuple(
                workGroupSize.x, workGroupSize.y, 1, kMaxLoadCount.x, kMaxLoadCount.y);

        const ShaderModule shaderModule = VulkanContext::shaderManager->CreateShaderModule(
                vk::ShaderStageFlagBits::eCompute, kLocationShaderPath, specializationValues);

        const vk::PushConstantRange pushConstantRange{
            vk::ShaderStageFlagBits::eCompute, 0, sizeof(glm::uvec2)
        };

        const ComputePipeline::Description pipelineDescription{
            shaderModule, layouts, { pushConstantRange }
        };

        std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(pipelineDescription);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

        return pipeline;
    }

    std::unique_ptr<ComputePipeline> CreateParametersPipeline(const std::vector<vk::DescriptorSetLayout> layouts)
    {
        const std::tuple specializationValues = std::make_tuple(kLuminanceBlockSize.x, kLuminanceBlockSize.y, 1);

        const ShaderModule shaderModule = VulkanContext::shaderManager->CreateShaderModule(
                vk::ShaderStageFlagBits::eCompute, kParametersShaderPath, specializationValues);

        const ComputePipeline::Description pipelineDescription{
            shaderModule, layouts, {}
        };

        std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(pipelineDescription);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

        return pipeline;
    }

    vk::DescriptorSet AllocatePanoramaDescriptorSet(vk::DescriptorSetLayout layout, vk::ImageView panoramaView)
    {
        const DescriptorPool& descriptorPool = *VulkanContext::descriptorPool;

        const vk::DescriptorSet descriptorSet = descriptorPool.AllocateDescriptorSets({ layout }).front();

        const DescriptorData descriptorData = DescriptorHelpers::GetData(panoramaView);

        descriptorPool.UpdateDescriptorSet(descriptorSet, { descriptorData }, 0);

        return descriptorSet;
    }

    LuminanceData CreateLuminanceData(vk::DescriptorSetLayout layout, const vk::Extent2D& panoramaExtent)
    {
        const vk::Extent2D extent(
                panoramaExtent.width / Details::kLuminanceBlockSize.x,
                panoramaExtent.height / Details::kLuminanceBlockSize.y);

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

        const DescriptorPool& descriptorPool = *VulkanContext::descriptorPool;

        const DescriptorData descriptorData = DescriptorHelpers::GetData(view);

        const vk::DescriptorSet descriptorSet = descriptorPool.AllocateDescriptorSets({ layout }).front();

        descriptorPool.UpdateDescriptorSet(descriptorSet, { descriptorData }, 0);

        return LuminanceData{ Texture{ image, view }, descriptorSet };
    }

    LocationData CreateLocationData(vk::DescriptorSetLayout layout)
    {
        const BufferDescription bufferDescription{
            sizeof(glm::uvec2),
            vk::BufferUsageFlagBits::eStorageBuffer,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        const vk::Buffer buffer = VulkanContext::bufferManager->CreateBuffer(
                bufferDescription, BufferCreateFlags::kNone);

        const DescriptorPool& descriptorPool = *VulkanContext::descriptorPool;

        const BufferInfo bufferInfo{ vk::DescriptorBufferInfo(buffer, 0, VK_WHOLE_SIZE) };

        const DescriptorData descriptorData{
            vk::DescriptorType::eStorageBuffer,
            bufferInfo
        };

        const vk::DescriptorSet descriptorSet = descriptorPool.AllocateDescriptorSets({ layout }).front();

        descriptorPool.UpdateDescriptorSet(descriptorSet, { descriptorData }, 0);

        return LocationData{ buffer, descriptorSet };
    }

    ParametersData CreateParametersData(vk::DescriptorSetLayout layout)
    {
        const vk::MemoryPropertyFlags memoryProperties
                = vk::MemoryPropertyFlagBits::eDeviceLocal
                | vk::MemoryPropertyFlagBits::eHostVisible
                | vk::MemoryPropertyFlagBits::eHostCoherent;

        const BufferDescription bufferDescription{
            sizeof(ShaderData::DirectLight),
            vk::BufferUsageFlagBits::eStorageBuffer,
            memoryProperties
        };

        const vk::Buffer buffer = VulkanContext::bufferManager->CreateBuffer(
                bufferDescription, BufferCreateFlags::kNone);

        const DescriptorPool& descriptorPool = *VulkanContext::descriptorPool;

        const BufferInfo bufferInfo{ vk::DescriptorBufferInfo(buffer, 0, VK_WHOLE_SIZE) };

        const DescriptorData descriptorData{
            vk::DescriptorType::eStorageBuffer,
            bufferInfo
        };

        const vk::DescriptorSet descriptorSet = descriptorPool.AllocateDescriptorSets({ layout }).front();

        descriptorPool.UpdateDescriptorSet(descriptorSet, { descriptorData }, 0);

        return ParametersData{ buffer, descriptorSet };
    }

    ShaderData::DirectLight RetrieveDirectLight(vk::Buffer parametersBuffer)
    {
        const MemoryManager& memoryManager = *VulkanContext::memoryManager;

        const MemoryBlock memoryBlock = memoryManager.GetBufferMemoryBlock(parametersBuffer);
        const ByteAccess parameters = memoryManager.MapMemory(memoryBlock);

        const ShaderData::DirectLight directLight = *reinterpret_cast<const ShaderData::DirectLight*>(parameters.data);

        memoryManager.UnmapMemory(memoryBlock);

        return directLight;
    }
}

DirectLighting::DirectLighting()
{
    DescriptorPool& descriptorPool = *VulkanContext::descriptorPool;

    const DescriptorDescription storageImageDescriptorDescription{
        1, vk::DescriptorType::eStorageImage,
        vk::ShaderStageFlagBits::eCompute,
        vk::DescriptorBindingFlags()
    };

    layouts.panorama = descriptorPool.CreateDescriptorSetLayout({ storageImageDescriptorDescription });
    layouts.luminance = descriptorPool.CreateDescriptorSetLayout({ storageImageDescriptorDescription });

    const DescriptorDescription storageBufferDescriptorDescription{
        1, vk::DescriptorType::eStorageBuffer,
        vk::ShaderStageFlagBits::eCompute,
        vk::DescriptorBindingFlags()
    };

    layouts.location = descriptorPool.CreateDescriptorSetLayout({ storageBufferDescriptorDescription });
    layouts.parameters = descriptorPool.CreateDescriptorSetLayout({ storageBufferDescriptorDescription });

    luminancePipeline = Details::CreateLuminancePipeline({ layouts.panorama, layouts.luminance });
    locationPipeline = Details::CreateLocationPipeline({ layouts.luminance, layouts.location });
    parametersPipeline = Details::CreateParametersPipeline({ layouts.panorama, layouts.location, layouts.parameters });
}

DirectLighting::~DirectLighting()
{
    VulkanContext::descriptorPool->DestroyDescriptorSetLayout(layouts.panorama);
    VulkanContext::descriptorPool->DestroyDescriptorSetLayout(layouts.luminance);
    VulkanContext::descriptorPool->DestroyDescriptorSetLayout(layouts.location);
}

ShaderData::DirectLight DirectLighting::RetrieveDirectLight(const Texture& panoramaTexture)
{
    const vk::Extent2D& panoramaExtent = VulkanHelpers::GetExtent2D(
            VulkanContext::imageManager->GetImageDescription(panoramaTexture.image).extent);

    Assert(panoramaExtent.width % Details::kLuminanceBlockSize.x == 0);
    Assert(panoramaExtent.height % Details::kLuminanceBlockSize.y == 0);

    const vk::ImageView panoramaView = VulkanContext::imageManager->CreateView(
            panoramaTexture.image, vk::ImageViewType::e2D, ImageHelpers::kFlatColor);

    const vk::DescriptorSet panoramaDescriptorSet
            = Details::AllocatePanoramaDescriptorSet(layouts.panorama, panoramaView);

    const Details::LuminanceData luminanceData = Details::CreateLuminanceData(layouts.luminance, panoramaExtent);
    const Details::LocationData locationData = Details::CreateLocationData(layouts.location);
    const Details::ParametersData parametersData = Details::CreateParametersData(layouts.location);

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
                panoramaDescriptorSet, luminanceData.descriptorSet
            };

            commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, luminancePipeline->Get());

            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                    luminancePipeline->GetLayout(), 0, luminanceDescriptorSets, {});

            const glm::uvec2 luminanceGroupCount(
                    panoramaExtent.width / Details::kLuminanceBlockSize.x,
                    panoramaExtent.height / Details::kLuminanceBlockSize.y);

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
                luminanceData.descriptorSet, locationData.descriptorSet
            };

            const glm::uvec2 loadCount = Details::CalculateLoadCount(luminanceGroupCount);

            commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, locationPipeline->Get());

            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                    locationPipeline->GetLayout(), 0, locationDescriptorSets, {});

            commandBuffer.pushConstants<glm::uvec2>(locationPipeline->GetLayout(),
                    vk::ShaderStageFlagBits::eCompute, 0, { loadCount });

            commandBuffer.dispatch(1, 1, 1);

            {
                const PipelineBarrier barrier{
                    SyncScope::kComputeShaderWrite,
                    SyncScope::kComputeShaderRead
                };

                BufferHelpers::InsertPipelineBarrier(commandBuffer, locationData.buffer, barrier);
            }

            const std::vector<vk::DescriptorSet> parametersDescriptorSets{
                panoramaDescriptorSet, locationData.descriptorSet, parametersData.descriptorSet
            };

            commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, parametersPipeline->Get());

            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                    parametersPipeline->GetLayout(), 0, parametersDescriptorSets, {});

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

    const ShaderData::DirectLight directLight = Details::RetrieveDirectLight(parametersData.buffer);

    VulkanContext::descriptorPool->FreeDescriptorSets({
        panoramaDescriptorSet, luminanceData.descriptorSet,
        locationData.descriptorSet, parametersData.descriptorSet
    });

    VulkanContext::textureManager->DestroyTexture(luminanceData.texture);
    VulkanContext::bufferManager->DestroyBuffer(locationData.buffer);
    VulkanContext::bufferManager->DestroyBuffer(parametersData.buffer);

    return directLight;
}

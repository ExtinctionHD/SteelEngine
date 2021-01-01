#include "Engine/Scene/Environment.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/ComputePipeline.hpp"

namespace Details
{
    struct LuminanceMap
    {
        Texture texture;
        glm::uvec2 size;
        DescriptorSet descriptorSet;
    };

    struct LightGroupID
    {
        vk::Buffer buffer;
        DescriptorSet descriptorSet;
    };

    constexpr glm::uvec2 kBlockSize(8, 8);

    const Filepath kSumShaderPath("~/Shaders/Compute/DirectLightSum.comp");
    const Filepath kMaxShaderPath("~/Shaders/Compute/DirectLightMax.comp");

    DescriptorSet CreatePanoramaDescriptorSet(const Texture& texture)
    {
        const DescriptorDescription descriptorDescription{
            1, vk::DescriptorType::eStorageImage,
            vk::ShaderStageFlagBits::eCompute,
            vk::DescriptorBindingFlags()
        };

        const DescriptorData descriptorData = DescriptorHelpers::GetData(texture.view);

        return DescriptorHelpers::CreateDescriptorSet({ descriptorDescription }, { descriptorData });
    }

    LuminanceMap CreateLuminanceMap(const vk::Extent2D& panoramaExtent)
    {
        const vk::Extent2D extent(
                panoramaExtent.width / kBlockSize.x,
                panoramaExtent.height / kBlockSize.y);

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

        const glm::uvec2 size(extent.width, extent.height);

        const DescriptorDescription descriptorDescription{
            1, vk::DescriptorType::eStorageImage,
            vk::ShaderStageFlagBits::eCompute,
            vk::DescriptorBindingFlags()
        };

        const DescriptorData descriptorData = DescriptorHelpers::GetData(view);

        const DescriptorSet descriptorSet = DescriptorHelpers::CreateDescriptorSet(
                { descriptorDescription }, { descriptorData });

        return LuminanceMap{ Texture{ image, view }, size, descriptorSet };
    }

    LightGroupID CreateLightGroupID()
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

        return LightGroupID{ buffer, descriptorSet };
    }

    std::unique_ptr<ComputePipeline> CreateComputePipeline(const Filepath& path,
            const glm::uvec2 workGroupSize, const std::vector<vk::DescriptorSetLayout> layouts)
    {
        const std::tuple specializationValues = std::make_tuple(workGroupSize.x, workGroupSize.y, 1);

        const ShaderModule shaderModule = VulkanContext::shaderManager->CreateShaderModule(
                vk::ShaderStageFlagBits::eCompute, path, specializationValues);

        const ComputePipeline::Description pipelineDescription{
            shaderModule, layouts, {}
        };

        std::unique_ptr<ComputePipeline> pipeline = ComputePipeline::Create(pipelineDescription);

        VulkanContext::shaderManager->DestroyShaderModule(shaderModule);

        return pipeline;
    }

    glm::vec4 CalculateLightDirection(const Texture& panoramaTexture, const vk::Extent2D& panoramaExtent)
    {
        Assert(panoramaExtent.width % kBlockSize.x == 0);
        Assert(panoramaExtent.height % kBlockSize.y == 0);

        const DescriptorSet panoramaDescriptorSet = CreatePanoramaDescriptorSet(panoramaTexture);

        const LuminanceMap luminanceMap = CreateLuminanceMap(panoramaExtent);
        const LightGroupID lightGroupID = CreateLightGroupID();

        std::unique_ptr<ComputePipeline> sumPipeline = CreateComputePipeline(kSumShaderPath, kBlockSize,
                { panoramaDescriptorSet.layout, luminanceMap.descriptorSet.layout });

        std::unique_ptr<ComputePipeline> maxPipeline = CreateComputePipeline(kMaxShaderPath, luminanceMap.size,
                { luminanceMap.descriptorSet.layout, lightGroupID.descriptorSet.layout });

        return {};
    }
}

Environment::Environment(const Filepath& path)
{
    const Texture panoramaTexture = VulkanContext::textureManager->CreateTexture(path);

    const vk::Extent2D& panoramaExtent = VulkanHelpers::GetExtent2D(
            VulkanContext::imageManager->GetImageDescription(panoramaTexture.image).extent);

    const vk::Extent2D environmentExtent = vk::Extent2D(panoramaExtent.height / 2, panoramaExtent.height / 2);

    texture = VulkanContext::textureManager->CreateCubeTexture(panoramaTexture, environmentExtent);
    lightDirection = Details::CalculateLightDirection(panoramaTexture, panoramaExtent);

    VulkanContext::textureManager->DestroyTexture(panoramaTexture);
}

Environment::~Environment()
{
    VulkanContext::textureManager->DestroyTexture(texture);
}

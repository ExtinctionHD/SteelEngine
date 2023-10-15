#include "Engine/Render/Vulkan/Resources/TextureManager.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"
#include "Engine/Filesystem/ImageLoader.hpp"

namespace Details
{
    static constexpr vk::Format kColorFormat = vk::Format::eR8G8B8A8Unorm;

    static void UpdateImage(vk::CommandBuffer commandBuffer, vk::Image image,
            const ImageDescription& description, const ByteView& data)
    {
        Assert(data.size == ImageHelpers::CalculateMipLevelSize(description, 0));

        const ImageUpdateRegion2D imageUpdateRegion{
            .layers = ImageHelpers::GetSubresourceLayers(description, 0),
            .extent = description.extent,
            .data = data,
        };

        const ImageLayoutTransition layoutTransition{
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal,
            PipelineBarrier{
                SyncScope::kWaitForNone,
                SyncScope::kTransferWrite
            }
        };

        ImageHelpers::TransitImageLayout(commandBuffer, image,
                ImageHelpers::GetSubresourceRange(description), layoutTransition);

        VulkanContext::imageManager->UpdateImage(commandBuffer, image, { imageUpdateRegion });
    }
}

BaseImage TextureManager::CreateTexture(const Filepath& filepath) const
{
    EASY_FUNCTION()

    const ImageSource imageSource = ImageLoader::LoadImage(filepath, 4);

    const BaseImage texture = CreateTexture(imageSource.format, imageSource.extent, imageSource.data);

    ImageLoader::FreeImage(imageSource.data.data);

    return texture;
}

BaseImage TextureManager::CreateTexture(vk::Format format, const vk::Extent2D& extent, const ByteView& data) const
{
    EASY_FUNCTION()

    const uint32_t mipLevelCount = ImageHelpers::CalculateMipLevelCount(extent);

    const vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage
            | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;

    const ImageDescription description{
        .format = format,
        .extent = extent,
        .mipLevelCount = mipLevelCount,
        .usage = usage,
        .stagingBuffer = true,
    };

    const BaseImage texture = VulkanContext::imageManager->CreateBaseImage(description);

    VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
        {
            Details::UpdateImage(commandBuffer, texture.image, description, data);

            if (description.mipLevelCount > 1)
            {
                ImageHelpers::GenerateMipLevels(commandBuffer, texture.image,
                        vk::ImageLayout::eTransferDstOptimal,
                        vk::ImageLayout::eShaderReadOnlyOptimal);
            }
            else
            {
                const ImageLayoutTransition layoutTransition{
                    vk::ImageLayout::eTransferDstOptimal,
                    vk::ImageLayout::eShaderReadOnlyOptimal,
                    PipelineBarrier{
                        SyncScope::kTransferWrite,
                        SyncScope::kBlockNone
                    }
                };

                ImageHelpers::TransitImageLayout(commandBuffer, texture.image,
                        ImageHelpers::GetSubresourceRange(description), layoutTransition);
            }
        });

    return texture;
}

CubeImage TextureManager::CreateCubeTexture(const BaseImage& panoramaTexture, const vk::Extent2D& extent) const
{
    EASY_FUNCTION()

    constexpr vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled
            | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;

    return panoramaToCube.GenerateCubeImage(panoramaTexture, extent, usage, vk::ImageLayout::eShaderReadOnlyOptimal);
}

BaseImage TextureManager::CreateColorTexture(const glm::vec4& color) const
{
    const Unorm4 data = ImageHelpers::FloatToUnorm(color);

    return CreateTexture(Details::kColorFormat, vk::Extent2D(1, 1), GetByteView(data));
}

vk::Sampler TextureManager::CreateSampler(const SamplerDescription& description) const
{
    const vk::SamplerCreateInfo createInfo({},
            description.magFilter, description.minFilter,
            description.mipmapMode, description.addressMode,
            description.addressMode, description.addressMode, 0.0f,
            description.maxAnisotropy.has_value(),
            description.maxAnisotropy.value_or(0.0f),
            false, vk::CompareOp(),
            description.minLod, description.maxLod,
            vk::BorderColor::eFloatTransparentBlack,
            description.unnormalizedCoords);

    const auto& [result, sampler] = VulkanContext::device->Get().createSampler(createInfo);
    Assert(result == vk::Result::eSuccess);

    return sampler;
}

void TextureManager::DestroyTexture(const BaseImage& texture) const
{
    VulkanContext::imageManager->DestroyImage(texture.image);
}

void TextureManager::DestroySampler(vk::Sampler sampler) const
{
    VulkanContext::device->Get().destroySampler(sampler);
}

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Engine/Render/Vulkan/Resources/TextureManager.hpp"

#include "Engine/Render/Renderer.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"

namespace Details
{
    static constexpr vk::Format kLDRFormat = vk::Format::eR8G8B8A8Unorm;
    static constexpr vk::Format kHDRFormat = vk::Format::eR32G32B32A32Sfloat;

    static uint8_t FloatToUnorm(float value)
    {
        const float min = static_cast<float>(std::numeric_limits<uint8_t>::min());
        const float max = static_cast<float>(std::numeric_limits<uint8_t>::max());
        return static_cast<uint8_t>(std::clamp(max * value, min, max));
    }

    static void UpdateImage(vk::CommandBuffer commandBuffer, vk::Image image,
            const ImageDescription& description, const ByteView& data)
    {
        Assert(data.size == ImageHelpers::CalculateBaseMipLevelSize(description));

        const vk::ImageSubresourceRange fullImage(vk::ImageAspectFlagBits::eColor,
                0, description.mipLevelCount, 0, description.layerCount);

        const vk::ImageSubresourceLayers baseMipLevel = ImageHelpers::GetSubresourceLayers(fullImage, 0);

        const ImageUpdate imageUpdate{
            baseMipLevel, { 0, 0, 0 },
            description.extent, data
        };

        const ImageLayoutTransition layoutTransition{
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal,
            PipelineBarrier{
                SyncScope::kWaitForNone,
                SyncScope::kTransferWrite
            }
        };

        ImageHelpers::TransitImageLayout(commandBuffer, image, fullImage, layoutTransition);

        VulkanContext::imageManager->UpdateImage(commandBuffer, image, { imageUpdate });
    }

    static void TransitImageLayoutAfterMipmapsGenerating(vk::CommandBuffer commandBuffer,
            vk::Image image, const vk::ImageSubresourceRange& subresourceRange)
    {
        const vk::ImageSubresourceRange lastMipLevel(vk::ImageAspectFlagBits::eColor,
                subresourceRange.levelCount - 1, 1,
                subresourceRange.baseArrayLayer, subresourceRange.layerCount);

        const vk::ImageSubresourceRange exceptLastMipLevel(vk::ImageAspectFlagBits::eColor,
                subresourceRange.baseMipLevel, subresourceRange.levelCount - 1,
                subresourceRange.baseArrayLayer, subresourceRange.layerCount);

        {
            const ImageLayoutTransition layoutTransition{
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                PipelineBarrier{
                    SyncScope::kTransferRead,
                    SyncScope::kShaderRead
                }
            };

            ImageHelpers::TransitImageLayout(commandBuffer, image, exceptLastMipLevel, layoutTransition);
        }

        {
            const ImageLayoutTransition layoutTransition{
                vk::ImageLayout::eUndefined,
                vk::ImageLayout::eShaderReadOnlyOptimal,
                PipelineBarrier{
                    SyncScope::kTransferWrite,
                    SyncScope::kShaderRead
                }
            };

            ImageHelpers::TransitImageLayout(commandBuffer, image, lastMipLevel, layoutTransition);
        }
    }
}

Texture TextureManager::CreateTexture(const Filepath& filepath) const
{
    ByteAccess data;
    int32_t width, height;

    const bool isHDR = stbi_is_hdr(filepath.GetAbsolute().c_str());
    if (isHDR)
    {
        float* hdrData = stbi_loadf(filepath.GetAbsolute().c_str(), &width, &height, nullptr, STBI_rgb_alpha);

        data.data = reinterpret_cast<uint8_t*>(hdrData);
        data.size = static_cast<uint32_t>(width * height) * STBI_rgb_alpha * sizeof(float);
    }
    else
    {
        data.data = stbi_load(filepath.GetAbsolute().c_str(), &width, &height, nullptr, STBI_rgb_alpha);
        data.size = static_cast<uint32_t>(width * height) * STBI_rgb_alpha * sizeof(uint8_t);
    }
    Assert(data.data != nullptr);

    const vk::Format format = isHDR ? Details::kHDRFormat : Details::kLDRFormat;
    const vk::Extent2D extent = VulkanHelpers::GetExtent(width, height);

    const Texture texture = CreateTexture(format, extent, data);

    stbi_image_free(data.data);

    return texture;
}

Texture TextureManager::CreateTexture(vk::Format format, const vk::Extent2D& extent, const ByteView& data) const
{
    const vk::Extent3D extent3D = VulkanHelpers::GetExtent3D(extent);
    const uint32_t mipLevelCount = ImageHelpers::CalculateMipLevelCount(extent);

    const vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage
            | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;

    const ImageDescription imageDescription{
        ImageType::e2D, format, extent3D,
        mipLevelCount, 1, vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal, usage,
        vk::ImageLayout::eUndefined,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    };

    const vk::Image image = VulkanContext::imageManager->CreateImage(imageDescription,
            ImageCreateFlagBits::eStagingBuffer);

    const vk::ImageSubresourceRange fullImage(vk::ImageAspectFlagBits::eColor,
            0, imageDescription.mipLevelCount, 0, imageDescription.layerCount);

    VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
        {
            Details::UpdateImage(commandBuffer, image, imageDescription, data);

            if (imageDescription.mipLevelCount > 1)
            {
                const vk::ImageSubresourceRange baseMipLevel(vk::ImageAspectFlagBits::eColor,
                        0, 1, 0, imageDescription.layerCount);

                const ImageLayoutTransition layoutTransition{
                    vk::ImageLayout::eTransferDstOptimal,
                    vk::ImageLayout::eTransferSrcOptimal,
                    PipelineBarrier{
                        SyncScope::kTransferWrite,
                        SyncScope::kTransferRead
                    }
                };

                ImageHelpers::TransitImageLayout(commandBuffer, image, baseMipLevel, layoutTransition);

                ImageHelpers::GenerateMipmaps(commandBuffer, image, extent3D, fullImage);

                Details::TransitImageLayoutAfterMipmapsGenerating(commandBuffer, image, fullImage);
            }
            else
            {
                const ImageLayoutTransition layoutTransition{
                    vk::ImageLayout::eTransferDstOptimal,
                    vk::ImageLayout::eShaderReadOnlyOptimal,
                    PipelineBarrier{
                        SyncScope::kTransferWrite,
                        SyncScope::kShaderRead
                    }
                };

                ImageHelpers::TransitImageLayout(commandBuffer, image, fullImage, layoutTransition);
            }
        });

    const vk::ImageView view = VulkanContext::imageManager->CreateView(image, vk::ImageViewType::e2D, fullImage);

    return Texture{ image, view };
}

Texture TextureManager::CreateCubeTexture(const Texture& panoramaTexture, const vk::Extent2D& extent) const
{
    const vk::Format format = VulkanContext::imageManager->GetImageDescription(panoramaTexture.image).format;
    const vk::Extent3D extent3D = VulkanHelpers::GetExtent3D(extent);
    const vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled
            | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;

    const ImageDescription imageDescription{
        ImageType::eCube, format, extent3D,
        ImageHelpers::CalculateMipLevelCount(extent),
        ImageHelpers::kCubeFaceCount,
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal, usage,
        vk::ImageLayout::eUndefined,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    };

    const vk::Image cubeImage = VulkanContext::imageManager->CreateImage(imageDescription, ImageCreateFlags::kNone);

    panoramaToCube.Convert(panoramaTexture, cubeImage, extent);

    VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
        {
            const vk::ImageSubresourceRange baseMipLevel(vk::ImageAspectFlagBits::eColor,
                    0, 1, 0, imageDescription.layerCount);

            const vk::ImageSubresourceRange mipLevels(vk::ImageAspectFlagBits::eColor,
                    1, imageDescription.mipLevelCount - 1, 0, imageDescription.layerCount);

            const vk::ImageSubresourceRange fullImage(vk::ImageAspectFlagBits::eColor,
                    0, imageDescription.mipLevelCount, 0, imageDescription.layerCount);

            {
                const ImageLayoutTransition layoutTransition{
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eTransferSrcOptimal,
                    PipelineBarrier{
                        SyncScope::kWaitForNone,
                        SyncScope::kTransferRead
                    }
                };

                ImageHelpers::TransitImageLayout(commandBuffer,
                    cubeImage, baseMipLevel, layoutTransition);
            }

            {
                const ImageLayoutTransition layoutTransition{
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eTransferDstOptimal,
                    PipelineBarrier{
                        SyncScope::kWaitForNone,
                        SyncScope::kTransferRead
                    }
                };

                ImageHelpers::TransitImageLayout(commandBuffer,
                    cubeImage, mipLevels, layoutTransition);
            }

            ImageHelpers::GenerateMipmaps(commandBuffer, cubeImage, extent3D, fullImage);

            Details::TransitImageLayoutAfterMipmapsGenerating(commandBuffer, cubeImage, fullImage);
        });

    const vk::ImageView cubeView = VulkanContext::imageManager->CreateView(
            cubeImage, vk::ImageViewType::eCube, ImageHelpers::kCubeColor);

    return Texture{ cubeImage, cubeView };
}

Texture TextureManager::CreateColorTexture(const glm::vec4& color) const
{
    const std::array<uint8_t, glm::vec4::length()> data{
        Details::FloatToUnorm(color.r),
        Details::FloatToUnorm(color.g),
        Details::FloatToUnorm(color.b),
        Details::FloatToUnorm(color.a)
    };

    return CreateTexture(Details::kLDRFormat, vk::Extent2D(1, 1), ByteView(data));
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
            vk::BorderColor::eFloatOpaqueBlack, false);

    const auto& [result, sampler] = VulkanContext::device->Get().createSampler(createInfo);
    Assert(result == vk::Result::eSuccess);

    return sampler;
}

void TextureManager::DestroyTexture(const Texture& texture) const
{
    VulkanContext::imageManager->DestroyImage(texture.image);
}

void TextureManager::DestroySampler(vk::Sampler sampler) const
{
    VulkanContext::device->Get().destroySampler(sampler);
}

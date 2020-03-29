#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Engine/Render/Vulkan/Resources/TextureCache.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

namespace STextureCache
{
    uint32_t CalculateMipLevelCount(int32_t width, int32_t height)
    {
        return static_cast<uint32_t>(std::ceil(std::log2(std::max(width, height))));
    }

    void UpdateImage(vk::CommandBuffer commandBuffer, vk::Image image,
            const ImageDescription &description, const uint8_t *pixels)
    {
        const vk::ImageSubresourceRange fullImage(vk::ImageAspectFlagBits::eColor,
                0, description.mipLevelCount, 0, description.layerCount);

        const vk::ImageSubresourceLayers baseMipLevel = ImageHelpers::GetSubresourceLayers(fullImage, 0);

        const ByteView byteView{
            pixels, ImageHelpers::CalculateBaseMipLevelSize(description)
        };

        const ImageUpdate imageUpdate{
            byteView, baseMipLevel, { 0, 0, 0 }, description.extent,
        };

        const ImageLayoutTransition layoutTransition{
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal,
            PipelineBarrier{
                SyncScope{ vk::PipelineStageFlagBits::eTopOfPipe, vk::AccessFlags() },
                SyncScope::kTransferWrite
            }
        };

        ImageHelpers::TransitImageLayout(commandBuffer, image, fullImage, layoutTransition);

        VulkanContext::imageManager->UpdateImage(commandBuffer, image, { imageUpdate });
    }

    void GenerateMipmaps(vk::CommandBuffer commandBuffer, vk::Image image,
            const ImageDescription &description)
    {
        const vk::ImageSubresourceRange fullImage(vk::ImageAspectFlagBits::eColor,
                0, description.mipLevelCount, 0, description.layerCount);
        const vk::ImageSubresourceRange exceptLastMipLevel(vk::ImageAspectFlagBits::eColor,
                0, description.mipLevelCount - 1, 0, description.layerCount);
        const vk::ImageSubresourceRange baseMipLevel(vk::ImageAspectFlagBits::eColor,
                0, 1, 0, description.layerCount);
        const vk::ImageSubresourceRange lastMipLevel(vk::ImageAspectFlagBits::eColor,
                description.mipLevelCount - 1, 1, 0, description.layerCount);

        const ImageLayoutTransition dstToSrcLayoutTransition{
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::eTransferSrcOptimal,
            PipelineBarrier{
                SyncScope::kTransferWrite,
                SyncScope::kTransferRead
            }
        };

        ImageHelpers::TransitImageLayout(commandBuffer, image, baseMipLevel, dstToSrcLayoutTransition);

        ImageHelpers::GenerateMipmaps(commandBuffer, image, description.extent, fullImage);

        const ImageLayoutTransition layoutTransition{
            vk::ImageLayout::eTransferSrcOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            PipelineBarrier{
                SyncScope::kTransferRead,
                SyncScope::kShaderRead
            }
        };

        ImageHelpers::TransitImageLayout(commandBuffer, image, exceptLastMipLevel, layoutTransition);

        const ImageLayoutTransition lastMipLevelLayoutTransition{
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            PipelineBarrier{
                SyncScope::kTransferWrite,
                SyncScope::kShaderRead
            }
        };

        ImageHelpers::TransitImageLayout(commandBuffer, image, lastMipLevel, lastMipLevelLayoutTransition);
    }
}

TextureCache::~TextureCache()
{
    for (auto &[description, sampler] : samplers)
    {
        VulkanContext::device->Get().destroySampler(sampler);
    }

    for (auto &[filepath, entry] : textures)
    {
        VulkanContext::imageManager->DestroyImage(entry.image);
    }
}

Texture TextureCache::GetTexture(const Filepath &filepath, const SamplerDescription &samplerDescription)
{
    TextureEntry &entry = textures[filepath];

    vk::Image &image = entry.image;
    vk::ImageView &view = entry.view;

    if (!image)
    {
        int32_t width, height;
        uint8_t *pixels = stbi_load(filepath.GetAbsolute().c_str(), &width, &height, nullptr, STBI_rgb_alpha);
        Assert(pixels != nullptr);

        const uint32_t mipLevelCount = STextureCache::CalculateMipLevelCount(width, height);

        const vk::Extent3D extent(static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1);

        const vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled
                | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;

        const ImageDescription description{
            ImageType::e2D, vk::Format::eR8G8B8A8Unorm, extent, mipLevelCount, 1,
            vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, usage,
            vk::ImageLayout::eUndefined, vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        image = VulkanContext::imageManager->CreateImage(description, ImageCreateFlagBits::eStagingBuffer);

        const vk::ImageSubresourceRange fullImage(vk::ImageAspectFlagBits::eColor,
                0, description.mipLevelCount, 0, description.layerCount);

        view = VulkanContext::imageManager->CreateView(image, fullImage);

        VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
            {
                STextureCache::UpdateImage(commandBuffer, image, description, pixels);

                if (mipLevelCount > 1)
                {
                    STextureCache::GenerateMipmaps(commandBuffer, image, description);
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

        stbi_image_free(pixels);
    }

    return { image, view, GetSampler(samplerDescription) };
}

vk::Sampler TextureCache::GetSampler(const SamplerDescription &description)
{
    const auto it = samplers.find(description);

    if (it != samplers.end())
    {
        return it->second;
    }

    const vk::SamplerCreateInfo createInfo({},
            description.magFilter, description.minFilter,
            description.mipmapMode, description.addressMode,
            description.addressMode, description.addressMode, 0.0f,
            description.maxAnisotropy.has_value(),
            description.maxAnisotropy.value_or(0.0f),
            false, vk::CompareOp(),
            description.minLod, description.maxLod,
            vk::BorderColor::eFloatOpaqueBlack, false);

    const auto &[result, sampler] = VulkanContext::device->Get().createSampler(createInfo);
    Assert(result == vk::Result::eSuccess);

    samplers.emplace(description, sampler);

    return sampler;
}

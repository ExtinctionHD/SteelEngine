#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Engine/Render/Vulkan/Resources/TextureCache.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

namespace STextureCache
{
    const ImageLayoutTransition kTextureLayoutTransition{
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        PipelineBarrier{
            SyncScope{
                vk::PipelineStageFlagBits::eTopOfPipe,
                vk::AccessFlags()
            },
            SyncScope{
                vk::PipelineStageFlagBits::eAllGraphics | vk::PipelineStageFlagBits::eRayTracingShaderNV,
                vk::AccessFlagBits::eShaderRead
            }
        }
    };

    uint32_t CalculateMipLevelCount(int width, int height)
    {
        return static_cast<uint32_t>(std::ceil(std::log2(std::max(width, height))));
    }

    void GenerateMipmaps(vk::Image image, const vk::Extent3D &extent,
            const vk::ImageSubresourceRange &subresourceRange)
    {
        VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
            {
                const ImageLayoutTransition layoutTransition{
                    vk::ImageLayout::eUndefined,
                    kTextureLayoutTransition.newLayout,
                    PipelineBarrier{
                        SyncScope{
                            vk::PipelineStageFlagBits::eTransfer,
                            vk::AccessFlagBits::eTransferWrite
                        },
                        kTextureLayoutTransition.pipelineBarrier.blockedScope
                    }
                };

                ImageHelpers::GenerateMipmaps(commandBuffer, image,
                        extent, subresourceRange, layoutTransition);
            });
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

    auto &[image, view] = entry;

    if (!image)
    {
        int width, height;
        stbi_uc *pixels = stbi_load(filepath.GetAbsolute().c_str(), &width, &height, nullptr, STBI_rgb_alpha);
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

        const uint8_t *data = reinterpret_cast<const uint8_t*>(pixels);
        Bytes bytes(data, data + width * height * STBI_rgb_alpha);

        stbi_image_free(pixels);

        ImageLayoutTransition layoutTransition = STextureCache::kTextureLayoutTransition;
        if (mipLevelCount > 1)
        {
            layoutTransition.newLayout = vk::ImageLayout::eTransferSrcOptimal;
            layoutTransition.pipelineBarrier.blockedScope = SyncScope{
                vk::PipelineStageFlagBits::eTransfer,
                vk::AccessFlagBits::eTransferRead
            };
        }

        const ImageUpdateRegion updateRegion{
            std::move(bytes), vk::ImageSubresource(vk::ImageAspectFlagBits::eColor, 0, 0),
            { 0, 0, 0 }, extent, layoutTransition
        };

        image = VulkanContext::imageManager->CreateImage(description,
                ImageCreateFlagBits::eStagingBuffer, { updateRegion });

        const vk::ImageSubresourceRange subresourceRange(vk::ImageAspectFlagBits::eColor,
                0, description.mipLevelCount, 0, description.layerCount);

        view = VulkanContext::imageManager->CreateView(image, subresourceRange);

        if (mipLevelCount > 1)
        {
            STextureCache::GenerateMipmaps(image, extent, subresourceRange);
        }
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

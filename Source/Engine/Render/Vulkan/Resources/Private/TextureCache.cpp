#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Engine/Render/Vulkan/Resources/TextureCache.hpp"

#include "Engine/Render/Vulkan/Resources/ImageManager.hpp"

namespace STextureCache
{
    const ImageLayoutTransition kTextureLayoutTransition{
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        PipelineBarrier{
            SynchronizationScope{
                vk::PipelineStageFlagBits::eTopOfPipe,
                vk::AccessFlags()
            },
            SynchronizationScope{
                vk::PipelineStageFlagBits::eAllGraphics,
                vk::AccessFlagBits::eShaderRead
            }
        }
    };

    vk::Format GetFormat(int channels)
    {
        switch (channels)
        {
        case STBI_grey:
            return vk::Format::eR8Unorm;
        case STBI_grey_alpha:
            return vk::Format::eR8G8Unorm;
        case STBI_rgb:
            return vk::Format::eR8G8B8Unorm;
        case STBI_rgb_alpha:
            return vk::Format::eR8G8B8A8Unorm;
        default:
            Assert(false);
            return vk::Format();
        }
    }
}

TextureCache::TextureCache(std::shared_ptr<Device> device_, std::shared_ptr<ImageManager> imageManager_)
    : device(device_)
    , imageManager(imageManager_)
{}

TextureCache::~TextureCache()
{
    for (auto &[description, sampler] : samplers)
    {
        device->Get().destroySampler(sampler);
    }

    for (auto &[filepath, image] : textures)
    {
        imageManager->DestroyImage(image);
    }
}

Texture TextureCache::GetTexture(const Filepath &filepath, const SamplerDescription &samplerDescription)
{
    ImageHandle &image = textures[filepath];
    if (image == nullptr)
    {
        int width, height, channels;
        const stbi_uc *pixels = stbi_load(filepath.GetAbsolute().c_str(), &width, &height, &channels, 0);
        Assert(pixels != nullptr);

        const vk::Extent3D extent(width, height, 1);
        const vk::Format format = STextureCache::GetFormat(channels);
        const ImageDescription description{
            ImageType::e2D, format, extent, 1, 1,
            vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
            vk::ImageLayout::eUndefined, vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        const uint8_t *data = reinterpret_cast<const uint8_t*>(pixels);
        Bytes bytes(data, data + width * height * channels);

        const ImageUpdateRegion updateRegion{
            std::move(bytes), vk::ImageSubresource(vk::ImageAspectFlagBits::eColor, 0, 0),
            { 0, 0, 0 }, extent, STextureCache::kTextureLayoutTransition
        };

        image = imageManager->CreateImage(description, ImageCreateFlagBits::eStagingBuffer, { updateRegion });

        const vk::ImageSubresourceRange subresourceRange(vk::ImageAspectFlagBits::eColor,
                0, description.mipLevelCount, 0, description.layerCount);

        imageManager->CreateView(image, subresourceRange);
    }

    return { image, GetSampler(samplerDescription) };
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

    const auto &[result, sampler] = device->Get().createSampler(createInfo);
    Assert(result == vk::Result::eSuccess);

    samplers.emplace(description, sampler);

    return sampler;
}

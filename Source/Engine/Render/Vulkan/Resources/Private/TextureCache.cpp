#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Engine/Render/Vulkan/Resources/TextureCache.hpp"

#include "Engine/Render/Vulkan/Resources/ImageManager.hpp"

namespace STextureCache
{
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
            return {};
        }
    }
}

TextureCache::TextureCache(std::shared_ptr<Device> aDevice, std::shared_ptr<ImageManager> aImageManager)
    : device(aDevice)
    , imageManager(aImageManager)
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
            eImageType::k2D, format, extent, 1, 1,
            vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
            vk::ImageLayout::eUndefined, vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        const vk::DeviceSize stagingBufferSize = width * height * channels;

        image = imageManager->CreateImage(description, stagingBufferSize);

        const vk::ImageSubresourceRange subresourceRange(vk::ImageAspectFlagBits::eColor,
                0, description.mipLevelCount, 0, description.layerCount);

        imageManager->CreateView(image, subresourceRange);

        const uint8_t *data = reinterpret_cast<const uint8_t *>(pixels);
        const Bytes bytes(data, data + width * height * channels);
        const ImageUpdateRegion updateRegion{
            bytes, vk::ImageSubresource(vk::ImageAspectFlagBits::eColor, 0, 0),
            vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal,
            { 0, 0, 0 }, extent
        };

        image->AddUpdateRegion(updateRegion);

        device->ExecuteOneTimeCommands([this, &image](vk::CommandBuffer commandBuffer)
            {
                imageManager->UpdateImage(image, commandBuffer);
            });
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
            false, {}, description.minLod, description.maxLod,
            vk::BorderColor::eFloatOpaqueBlack, false);

    const auto &[result, sampler] = device->Get().createSampler(createInfo);
    Assert(result == vk::Result::eSuccess);

    samplers.emplace(description, sampler);

    return sampler;
}

#include "Engine/Render/Vulkan/Resources/TextureCache.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Resources/ResourceContext.hpp"
#include "Engine/Filesystem/ImageLoader.hpp"

namespace Details
{
    constexpr SamplerDescription kLinearRepeatSamplerDescription{
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eLinear,
        .addressMode = vk::SamplerAddressMode::eRepeat,
    };

    constexpr SamplerDescription kDirectTexelSamplerDescription{
        .magFilter = vk::Filter::eNearest,
        .minFilter = vk::Filter::eNearest,
        .mipmapMode = vk::SamplerMipmapMode::eNearest,
        .addressMode = vk::SamplerAddressMode::eClampToBorder,
        .maxAnisotropy = 0.0f,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .unnormalizedCoords = true
    };

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

        ResourceContext::UpdateImage(commandBuffer, image, { imageUpdateRegion });
    }
}

TextureCache::TextureCache()
{
    defaultSamplerCache.emplace(SamplerType::eLinerRepeat, 
            GetSampler(Details::kLinearRepeatSamplerDescription));
    defaultSamplerCache.emplace(SamplerType::eDirectTexel, 
            GetSampler(Details::kDirectTexelSamplerDescription));
}

Texture TextureCache::GetTexture(const Filepath& filepath) const
{
    EASY_FUNCTION()

    const ImageSource imageSource = ImageLoader::LoadImage(filepath, 4);

    const Texture texture = CreateTexture(imageSource);

    ImageLoader::FreeImage(imageSource.data.data);

    return texture;
}

Texture TextureCache::CreateTexture(const ImageSource& source) const
{
    EASY_FUNCTION()

    const uint32_t mipLevelCount = ImageHelpers::CalculateMipLevelCount(source.extent);

    const vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage
            | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;

    const ImageDescription description{
        .format = source.format,
        .extent = source.extent,
        .mipLevelCount = mipLevelCount,
        .usage = usage,
        .stagingBuffer = true,
    };

    const BaseImage texture = ResourceContext::CreateBaseImage(description);

    VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
        {
            Details::UpdateImage(commandBuffer, texture.image, description, source.data);

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

    return Texture{ texture, GetSampler(SamplerType::eLinerRepeat) };
}

Texture TextureCache::CreateColorTexture(const glm::vec4& color) const
{
    const Unorm4 data = ImageHelpers::FloatToUnorm(color);

    const ImageSource imageSource{
        GetByteView(data),
        vk::Extent2D(1, 1),
        Details::kColorFormat
    };

    return CreateTexture(imageSource);
}

Texture TextureCache::CreateCubeTexture(const BaseImage& panoramaTexture, const vk::Extent2D& extent) const
{
    EASY_FUNCTION()

    constexpr vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled
            | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;

    return panoramaToCube.GenerateCubeImage(panoramaTexture, extent, usage, vk::ImageLayout::eShaderReadOnlyOptimal);
}

vk::Sampler TextureCache::GetSampler(const SamplerDescription& description)
{
    const auto it = samplerCache.find(description);

    if (it != samplerCache.end())
    {
        return it->second;
    }

    const vk::SamplerCreateInfo createInfo({},
            description.magFilter, description.minFilter,
            description.mipmapMode, description.addressMode,
            description.addressMode, description.addressMode, 0.0f,
            description.maxAnisotropy > 0.0f,
            description.maxAnisotropy,
            false, vk::CompareOp(),
            description.minLod, description.maxLod,
            vk::BorderColor::eFloatTransparentBlack,
            description.unnormalizedCoords);

    const auto& [result, sampler] = VulkanContext::device->Get().createSampler(createInfo);
    Assert(result == vk::Result::eSuccess);

    samplerCache.emplace(description, sampler);

    return sampler;
}

vk::Sampler TextureCache::GetSampler(SamplerType type) const
{
    return defaultSamplerCache.at(type);
}

void TextureCache::RemoveTextures(const Filepath& directory)
{
    
}

void TextureCache::DestroyTexture(const Texture& texture)
{
    
}

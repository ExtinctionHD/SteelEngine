#include "Engine/Render/Vulkan/Resources/TextureCache.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Resources/ResourceContext.hpp"
#include "Engine/Filesystem/ImageLoader.hpp"

namespace Details
{
    static const std::map<DefaultSampler, SamplerDescription> kSamplerDescriptions{
        {
            DefaultSampler::eLinearRepeat,
            SamplerDescription{
                .magFilter = vk::Filter::eLinear,
                .minFilter = vk::Filter::eLinear,
                .mipmapMode = vk::SamplerMipmapMode::eLinear,
                .addressMode = vk::SamplerAddressMode::eRepeat,
            }
        },
        {
            DefaultSampler::eTexelClamp,
            SamplerDescription{
                .magFilter = vk::Filter::eNearest,
                .minFilter = vk::Filter::eNearest,
                .mipmapMode = vk::SamplerMipmapMode::eNearest,
                .addressMode = vk::SamplerAddressMode::eClampToEdge,
                .maxAnisotropy = 0.0f,
                .minLod = 0.0f,
                .maxLod = 0.0f,
                .unnormalizedCoords = true,
            }
        },
    };

    static constexpr vk::Extent2D kDefaultTextureExtent(256, 256);

    static constexpr uint32_t kDefaultTextureSize = kDefaultTextureExtent.width * kDefaultTextureExtent.height;

    static constexpr vk::Format kDefaultTextureFormat = vk::Format::eR8G8B8A8Unorm;

    static constexpr vk::ImageUsageFlags kTextureUsage
            = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage
            | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;

    static constexpr std::array<Color, 2> kCheckeredTextureColors{ Color(255, 255, 255), Color(0, 0, 0) };

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

    static BaseImage CreateTextureImage(const ImageSourceView& source)
    {
        const uint32_t mipLevelCount = ImageHelpers::CalculateMipLevelCount(source.extent);

        const ImageDescription description{
            .format = source.format,
            .extent = source.extent,
            .mipLevelCount = mipLevelCount,
            .usage = Details::kTextureUsage,
            .stagingBuffer = true,
        };

        const BaseImage baseImage = ResourceContext::CreateBaseImage(description);

        VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
            {
                Details::UpdateImage(commandBuffer, baseImage.image, description, source.data);

                if (description.mipLevelCount > 1)
                {
                    ImageHelpers::GenerateMipLevels(commandBuffer, baseImage.image,
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

                    ImageHelpers::TransitImageLayout(commandBuffer, baseImage.image,
                            ImageHelpers::GetSubresourceRange(description), layoutTransition);
                }
            });

        return baseImage;
    }

    static BaseImage CreateCheckeredTexture()
    {
        std::vector<Color> checkeredTextureData;
        checkeredTextureData.reserve(kDefaultTextureSize);

        for (uint32_t x = 0; x < kDefaultTextureExtent.width; ++x)
        {
            const bool isLeftCorner = x < kDefaultTextureExtent.width / 2;

            for (uint32_t y = 0; y < kDefaultTextureExtent.height; ++y)
            {
                const bool isTopCorner = y < kDefaultTextureExtent.height / 2;

                if (isLeftCorner == isTopCorner)
                {
                    checkeredTextureData.push_back(kCheckeredTextureColors[0]);
                }
                else
                {
                    checkeredTextureData.push_back(kCheckeredTextureColors[1]);
                }
            }
        }

        const ImageSourceView source{
            GetByteView(checkeredTextureData),
            kDefaultTextureExtent,
            kDefaultTextureFormat,
        };

        return CreateTextureImage(source);
    }

    static std::map<DefaultTexture, BaseImage> CreateDefaultTextures()
    {
        const std::vector<Color> blackTextureData = Repeat(Color::kBlack, kDefaultTextureSize);
        const std::vector<Color> whiteTextureData = Repeat(Color::kWhite, kDefaultTextureSize);
        const std::vector<Color> normalTextureData = Repeat(Color(127, 127, 255), kDefaultTextureSize);

        std::map<DefaultTexture, BaseImage> defaultTextures;

        defaultTextures.emplace(DefaultTexture::eBlack, CreateTextureImage(ImageSourceView{
            GetByteView(blackTextureData),
            kDefaultTextureExtent,
            kDefaultTextureFormat,
        }));

        defaultTextures.emplace(DefaultTexture::eWhite, CreateTextureImage(ImageSourceView{
            GetByteView(whiteTextureData),
            kDefaultTextureExtent,
            kDefaultTextureFormat,
        }));

        defaultTextures.emplace(DefaultTexture::eNormal, CreateTextureImage(ImageSourceView{
            GetByteView(normalTextureData),
            kDefaultTextureExtent,
            kDefaultTextureFormat,
        }));

        defaultTextures.emplace(DefaultTexture::eCheckered, CreateCheckeredTexture());

        return defaultTextures;
    }

    static vk::Sampler CreateSampler(const SamplerDescription& description)
    {
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

        return sampler;
    }
}

std::unique_ptr<PanoramaToCube> TextureCache::panoramaToCube;

std::map<Filepath, TextureCache::TextureEntry> TextureCache::textureCache;
std::map<DefaultTexture, BaseImage> TextureCache::defaultTextures;

std::map<SamplerDescription, vk::Sampler> TextureCache::samplerCache;
std::map<DefaultSampler, vk::Sampler> TextureCache::defaultSamplers;

void TextureCache::Create()
{
    panoramaToCube = std::make_unique<PanoramaToCube>();

    defaultTextures = Details::CreateDefaultTextures();

    for (const auto& [type, description] : Details::kSamplerDescriptions)
    {
        const vk::Sampler sampler = Details::CreateSampler(description);

        defaultSamplers.emplace(type, sampler);

        samplerCache.emplace(description, sampler);
    }
}

void TextureCache::Destroy()
{
    panoramaToCube.reset();

    for (const auto& [path, entry] : textureCache)
    {
        Assert(entry.count == 0);

        ResourceContext::DestroyResource(entry.image);
    }

    for (const auto& [color, image] : defaultTextures)
    {
        ResourceContext::DestroyResource(image);
    }

    for (const auto& [description, sampler] : samplerCache)
    {
        ResourceContext::DestroyResource(sampler);
    }

    textureCache.clear();
    defaultTextures.clear();

    samplerCache.clear();
    defaultSamplers.clear();
}

Texture TextureCache::GetTexture(const Filepath& path)
{
    EASY_FUNCTION()

    const auto it = textureCache.find(path);

    if (it != textureCache.end())
    {
        it->second.count++;

        return Texture{ it->second.image, GetSampler() };
    }

    const ImageSource imageSource = ImageLoader::LoadImage(path, 4);

    const Texture texture = CreateTexture(imageSource);

    VulkanHelpers::SetObjectName(VulkanContext::device->Get(), texture.image.image, path.GetBaseName());

    ImageLoader::FreeImage(imageSource.data.data);

    textureCache.emplace(path, TextureEntry{ texture.image, 1 });

    return texture;
}

Texture TextureCache::GetTexture(DefaultTexture key)
{
    return Texture{ defaultTextures.at(key), GetSampler() };
}

Texture TextureCache::CreateTexture(const ImageSourceView& source)
{
    EASY_FUNCTION()

    return Texture{ Details::CreateTextureImage(source), GetSampler() };
}

Texture TextureCache::CreateCubeTexture(const BaseImage& panorama)
{
    EASY_FUNCTION()

    const BaseImage cubeImage = panoramaToCube->GenerateCubeImage(panorama,
            Details::kTextureUsage, vk::ImageLayout::eShaderReadOnlyOptimal);

    return Texture{ cubeImage, GetSampler() };
}

vk::Sampler TextureCache::GetSampler(const SamplerDescription& description)
{
    const auto it = samplerCache.find(description);

    if (it != samplerCache.end())
    {
        return it->second;
    }

    const vk::Sampler sampler = Details::CreateSampler(description);

    samplerCache.emplace(description, sampler);

    return sampler;
}

vk::Sampler TextureCache::GetSampler(DefaultSampler key)
{
    return defaultSamplers.at(key);
}

void TextureCache::ReleaseTexture(const Filepath& path, bool tryDestroy)
{
    const auto it = textureCache.find(path);

    if (it != textureCache.end())
    {
        Assert(it->second.count > 0);

        it->second.count--;

        if (tryDestroy && it->second.count == 0)
        {
            ResourceContext::DestroyResource(it->second.image);

            textureCache.erase(it);
        }
    }
}

void TextureCache::ReleaseTexture(const BaseImage& image, bool tryDestroy)
{
    const auto pred = [&](const std::pair<Filepath, TextureEntry>& pair)
        {
            return pair.second.image.image == image.image;
        };

    const auto it = std::ranges::find_if(textureCache, pred);

    if (it != textureCache.end())
    {
        Assert(it->second.count > 0);

        it->second.count--;

        if (tryDestroy && it->second.count == 0)
        {
            ResourceContext::DestroyResource(it->second.image);

            textureCache.erase(it);
        }
    }
}

bool TextureCache::TryDestroyTexture(const Filepath& path)
{
    const auto it = textureCache.find(path);

    if (it != textureCache.end())
    {
        if (it->second.count > 0)
        {
            ResourceContext::DestroyResource(it->second.image);

            textureCache.erase(it);

            return true;
        }

        return false;
    }

    return false;
}

bool TextureCache::TryDestroyTexture(const BaseImage& image)
{
    const auto pred = [&](const std::pair<Filepath, TextureEntry>& pair)
        {
            return pair.second.image.image == image.image;
        };

    const auto it = std::ranges::find_if(textureCache, pred);

    if (it != textureCache.end())
    {
        if (it->second.count > 0)
        {
            ResourceContext::DestroyResource(it->second.image);

            textureCache.erase(it);

            return true;
        }

        return false;
    }

    return false;
}

void TextureCache::DestroyUnusedTextures()
{
    for (auto it = textureCache.begin(); it != textureCache.end();)
    {
        if (it->second.count == 0)
        {
            ResourceContext::DestroyResourceSafe(it->second.image);

            it = textureCache.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

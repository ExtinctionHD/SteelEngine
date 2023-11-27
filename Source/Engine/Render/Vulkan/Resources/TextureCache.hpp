#pragma once

#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"

struct ImageSourceView;

class TextureCache
{
public:
    static void Create();
    static void Destroy();

    static Texture GetTexture(const Filepath& path);

    static Texture GetTexture(DefaultTexture key);

    static Texture CreateTexture(const ImageSourceView& source);

    static Texture CreateCubeTexture(const BaseImage& panorama);

    static vk::Sampler GetSampler(const SamplerDescription& description);

    static vk::Sampler GetSampler(DefaultSampler key = DefaultSampler::eLinearRepeat);

    static void ReleaseTexture(const Filepath& path, bool tryDestroy = false);

    static void ReleaseTexture(const BaseImage& image, bool tryDestroy = false);

    static bool TryDestroyTexture(const Filepath& path);

    static bool TryDestroyTexture(const BaseImage& image);

    static void DestroyUnusedTextures();

private:
    struct TextureEntry
    {
        BaseImage image;
        uint32_t count;
    };

    static std::unique_ptr<PanoramaToCube> panoramaToCube;

    static std::map<Filepath, TextureEntry> textureCache;
    static std::map<DefaultTexture, BaseImage> defaultTextures;

    static std::map<SamplerDescription, vk::Sampler> samplerCache;
    static std::map<DefaultSampler, vk::Sampler> defaultSamplers;
};

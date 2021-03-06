#pragma once

#include "Vulkan/Resources/TextureHelpers.hpp"

class DirectLighting;
class ImageBasedLighting;

class Renderer
{
public:
    static void Create();
    static void Destroy();

    static std::unique_ptr<DirectLighting> directLighting;
    static std::unique_ptr<ImageBasedLighting> imageBasedLighting;

    static vk::Sampler defaultSampler;
    static vk::Sampler texelSampler;

    static Texture blackTexture;
    static Texture whiteTexture;
    static Texture normalTexture;
};

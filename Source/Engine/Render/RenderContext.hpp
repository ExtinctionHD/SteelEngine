#pragma once

#include "Vulkan/Resources/TextureHelpers.hpp"

class DirectLighting;
class ImageBasedLighting;
class GlobalIllumination;

class RenderContext
{
public:
    static void Create();
    static void Destroy();

    static std::unique_ptr<DirectLighting> directLighting;
    static std::unique_ptr<ImageBasedLighting> imageBasedLighting;
    static std::unique_ptr<GlobalIllumination> globalIllumination;

    static vk::Sampler defaultSampler;
    static vk::Sampler texelSampler;

    static Texture blackTexture;
    static Texture whiteTexture;
    static Texture normalTexture;
};

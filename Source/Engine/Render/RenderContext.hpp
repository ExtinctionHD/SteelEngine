#pragma once

#include "Vulkan/Resources/ImageHelpers.hpp"

class FrameLoop;
class ImageBasedLighting;
class GlobalIllumination;

class RenderContext
{
public:
    static void Create();
    static void Destroy();

    static std::unique_ptr<FrameLoop> frameLoop;

    static std::unique_ptr<ImageBasedLighting> imageBasedLighting;
    static std::unique_ptr<GlobalIllumination> globalIllumination;

    static vk::Sampler defaultSampler;
    static vk::Sampler texelSampler;

    static BaseImage blackImage;
    static BaseImage whiteImage;
    static BaseImage normalImage;
};

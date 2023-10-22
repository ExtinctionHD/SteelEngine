#include "Engine/Render/RenderContext.hpp"

#include "Engine/Render/FrameLoop.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Resources/ResourceContext.hpp"
#include "Engine/Scene/ImageBasedLighting.hpp"
#include "Engine/Scene/GlobalIllumination.hpp"

namespace Details
{
    constexpr SamplerDescription kDefaultSamplerDescription{
        vk::Filter::eLinear,
        vk::Filter::eLinear,
        vk::SamplerMipmapMode::eLinear,
        vk::SamplerAddressMode::eRepeat,
        VulkanConfig::kMaxAnisotropy,
        0.0f, std::numeric_limits<float>::max(),
        false
    };

    constexpr SamplerDescription kTexelSamplerDescription{
        vk::Filter::eNearest,
        vk::Filter::eNearest,
        vk::SamplerMipmapMode::eNearest,
        vk::SamplerAddressMode::eClampToBorder,
        std::nullopt,
        0.0f, 0.0f,
        true
    };
}

std::unique_ptr<FrameLoop> RenderContext::frameLoop;
std::unique_ptr<ImageBasedLighting> RenderContext::imageBasedLighting;
std::unique_ptr<GlobalIllumination> RenderContext::globalIllumination;

vk::Sampler RenderContext::defaultSampler;
vk::Sampler RenderContext::texelSampler;

BaseImage RenderContext::blackImage;
BaseImage RenderContext::whiteImage;
BaseImage RenderContext::normalImage;

void RenderContext::Create()
{
    EASY_FUNCTION()

    frameLoop = std::make_unique<FrameLoop>();
    imageBasedLighting = std::make_unique<ImageBasedLighting>();
    globalIllumination = std::make_unique<GlobalIllumination>();

    const TextureManager& textureManager = *VulkanContext::textureManager;

    defaultSampler = textureManager.CreateSampler(Details::kDefaultSamplerDescription);
    texelSampler = textureManager.CreateSampler(Details::kTexelSamplerDescription);

    blackImage = textureManager.CreateColorTexture(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    whiteImage = textureManager.CreateColorTexture(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    normalImage = textureManager.CreateColorTexture(glm::vec4(0.5f, 0.5f, 1.0f, 1.0f));
}

void RenderContext::Destroy()
{
    ResourceContext::DestroyResource(defaultSampler);
    ResourceContext::DestroyResource(texelSampler);
    ResourceContext::DestroyResource(blackImage);
    ResourceContext::DestroyResource(whiteImage);
    ResourceContext::DestroyResource(normalImage);

    imageBasedLighting.reset();
    globalIllumination.reset();
    frameLoop.reset();
}

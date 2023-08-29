#include "Engine/Render/RenderContext.hpp"

#include "Engine/Render/FrameLoop.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Resources/ResourceHelpers.hpp"
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

Texture RenderContext::blackTexture;
Texture RenderContext::whiteTexture;
Texture RenderContext::normalTexture;

void RenderContext::Create()
{
    EASY_FUNCTION()

    frameLoop = std::make_unique<FrameLoop>();
    imageBasedLighting = std::make_unique<ImageBasedLighting>();
    globalIllumination = std::make_unique<GlobalIllumination>();

    const TextureManager& textureManager = *VulkanContext::textureManager;

    defaultSampler = textureManager.CreateSampler(Details::kDefaultSamplerDescription);
    texelSampler = textureManager.CreateSampler(Details::kTexelSamplerDescription);

    blackTexture = textureManager.CreateColorTexture(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    whiteTexture = textureManager.CreateColorTexture(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    normalTexture = textureManager.CreateColorTexture(glm::vec4(0.5f, 0.5f, 1.0f, 1.0f));
}

void RenderContext::Destroy()
{
    ResourceHelpers::DestroyResource(defaultSampler);
    ResourceHelpers::DestroyResource(texelSampler);
    ResourceHelpers::DestroyResource(blackTexture);
    ResourceHelpers::DestroyResource(whiteTexture);
    ResourceHelpers::DestroyResource(normalTexture);

    imageBasedLighting.reset();
    globalIllumination.reset();
    frameLoop.reset();
}

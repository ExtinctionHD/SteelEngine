#include "Engine/Render/Renderer.hpp"

#include "Engine/Render/Vulkan/VulkanConfig.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Scene/DirectLighting.hpp"
#include "Engine/Scene/ImageBasedLighting.hpp"

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

std::unique_ptr<DirectLighting> Renderer::directLighting;
std::unique_ptr<ImageBasedLighting> Renderer::imageBasedLighting;

vk::Sampler Renderer::defaultSampler;
vk::Sampler Renderer::texelSampler;

Texture Renderer::blackTexture;
Texture Renderer::whiteTexture;
Texture Renderer::normalTexture;

void Renderer::Create()
{
    directLighting = std::make_unique<DirectLighting>();
    imageBasedLighting = std::make_unique<ImageBasedLighting>();

    TextureManager& textureManager = *VulkanContext::textureManager;

    defaultSampler = textureManager.CreateSampler(Details::kDefaultSamplerDescription);
    texelSampler = textureManager.CreateSampler(Details::kTexelSamplerDescription);

    blackTexture = textureManager.CreateColorTexture(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    whiteTexture = textureManager.CreateColorTexture(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    normalTexture = textureManager.CreateColorTexture(glm::vec4(0.5f, 0.5f, 1.0f, 1.0f));
}

void Renderer::Destroy()
{
    TextureManager& textureManager = *VulkanContext::textureManager;
    textureManager.DestroySampler(defaultSampler);
    textureManager.DestroySampler(texelSampler);
    textureManager.DestroyTexture(blackTexture);
    textureManager.DestroyTexture(whiteTexture);
    textureManager.DestroyTexture(normalTexture);

    directLighting.reset();
    imageBasedLighting.reset();
}

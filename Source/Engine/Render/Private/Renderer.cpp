#include "Engine/Render/Renderer.hpp"

#include "Engine/Render/Vulkan/VulkanConfig.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"

std::unique_ptr<DirectLighting> Renderer::directLighting;

vk::Sampler Renderer::defaultSampler;

Texture Renderer::blackTexture;
Texture Renderer::whiteTexture;
Texture Renderer::normalTexture;

void Renderer::Create()
{
    directLighting = std::make_unique<DirectLighting>();

    TextureManager& textureManager = *VulkanContext::textureManager;

    defaultSampler = textureManager.CreateSampler(VulkanConfig::kDefaultSamplerDescription);

    blackTexture = textureManager.CreateColorTexture(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    whiteTexture = textureManager.CreateColorTexture(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    normalTexture = textureManager.CreateColorTexture(glm::vec4(0.5f, 0.5f, 1.0f, 1.0f));
}

void Renderer::Destroy()
{
    TextureManager& textureManager = *VulkanContext::textureManager;
    textureManager.DestroySampler(defaultSampler);
    textureManager.DestroyTexture(blackTexture);
    textureManager.DestroyTexture(whiteTexture);
    textureManager.DestroyTexture(normalTexture);

    directLighting.reset();
}

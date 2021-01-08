#include "Engine/Scene/Environment.hpp"

#include "Engine/Render/Renderer.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/ComputePipeline.hpp"
#include "Engine/Scene/DirectLighting.hpp"
#include "Engine/Scene/ImageBasedLighting.hpp"

namespace Details
{
    Texture CreateEnvironmentTexture(const Texture& panoramaTexture)
    {
        const vk::Extent2D& panoramaExtent = VulkanHelpers::GetExtent2D(
                VulkanContext::imageManager->GetImageDescription(panoramaTexture.image).extent);

        const vk::Extent2D environmentExtent = vk::Extent2D(panoramaExtent.height / 2, panoramaExtent.height / 2);

        return VulkanContext::textureManager->CreateCubeTexture(panoramaTexture, environmentExtent);
    }
}

Environment::Environment(const Filepath& path)
{
    const Texture panoramaTexture = VulkanContext::textureManager->CreateTexture(path);

    texture = Details::CreateEnvironmentTexture(panoramaTexture);
    directLight = Renderer::directLighting->RetrieveDirectLight(panoramaTexture);
    iblTextures = Renderer::imageBasedLighting->GenerateTextures(texture);

    VulkanContext::textureManager->DestroyTexture(panoramaTexture);
}

Environment::~Environment()
{
    VulkanContext::textureManager->DestroyTexture(texture);
    VulkanContext::textureManager->DestroyTexture(iblTextures.irradiance);
    VulkanContext::textureManager->DestroyTexture(iblTextures.reflection);
}

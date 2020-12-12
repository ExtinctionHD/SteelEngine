#include "Engine/Scene/Environment.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/ComputePipeline.hpp"

namespace Details
{

    Texture CreateEnvironmentTexture(const Filepath& path)
    {
        TextureManager& textureManager = *VulkanContext::textureManager;
        ImageManager& imageManager = *VulkanContext::imageManager;

        const Texture panoramaTexture = textureManager.CreateTexture(path);
        const vk::Extent3D& panoramaExtent = imageManager.GetImageDescription(panoramaTexture.image).extent;

        const vk::Extent2D environmentExtent = vk::Extent2D(panoramaExtent.height / 2, panoramaExtent.height / 2);
        const Texture environmentTexture = textureManager.CreateCubeTexture(panoramaTexture, environmentExtent);

        textureManager.DestroyTexture(panoramaTexture);

        return environmentTexture;
    }
}

Environment::Environment(const Filepath& path)
{
    texture = Details::CreateEnvironmentTexture(path);
}

Environment::~Environment()
{
    VulkanContext::textureManager->DestroyTexture(texture);
}

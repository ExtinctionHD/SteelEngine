#include "Engine/Scene/Environment.hpp"

#include "Engine/Render/Renderer.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/ComputePipeline.hpp"
#include "Engine/Scene/DirectLighting.hpp"
#include "Engine/Scene/ImageBasedLighting.hpp"

namespace Details
{
    static constexpr vk::Extent2D kMaxEnvironmentExtent(1024, 1024);

    static vk::Extent2D GetEnvironmentExtent(const vk::Extent2D& panoramaExtent)
    {
        Assert(panoramaExtent.width / panoramaExtent.height == 2);

        const uint32_t height = panoramaExtent.height / 2;

        if (height < kMaxEnvironmentExtent.height)
        {
            return vk::Extent2D(height, height);
        }

        return kMaxEnvironmentExtent;
    }

    static Texture CreateEnvironmentTexture(const Texture& panoramaTexture)
    {
        const vk::Extent2D& panoramaExtent = VulkanHelpers::GetExtent2D(
                VulkanContext::imageManager->GetImageDescription(panoramaTexture.image).extent);

        const vk::Extent2D environmentExtent = GetEnvironmentExtent(panoramaExtent);

        return VulkanContext::textureManager->CreateCubeTexture(panoramaTexture, environmentExtent);
    }
}

Environment::Environment(const Filepath& path)
{
    const Texture panoramaTexture = VulkanContext::textureManager->CreateTexture(path);

    texture = Details::CreateEnvironmentTexture(panoramaTexture);
    directLight = Renderer::directLighting->RetrieveDirectLight(panoramaTexture);
    iblTextures = Renderer::imageBasedLighting->GenerateTextures(texture);
    irradianceBuffer = Renderer::imageBasedLighting->GenerateIrradianceBuffer(texture);

    VulkanContext::textureManager->DestroyTexture(panoramaTexture);
}

Environment::~Environment()
{
    VulkanContext::textureManager->DestroyTexture(texture);
    VulkanContext::textureManager->DestroyTexture(iblTextures.irradiance);
    VulkanContext::textureManager->DestroyTexture(iblTextures.reflection);
    VulkanContext::bufferManager->DestroyBuffer(irradianceBuffer);
}

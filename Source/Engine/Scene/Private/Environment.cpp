#include "Engine/Scene/Environment.hpp"

#include "Engine/Render/RenderContext.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Scene/ImageBasedLighting.hpp"

namespace Details
{
    static constexpr vk::Extent2D kMaxCubemapExtent(1024, 1024);

    static vk::Extent2D GetCubemapExtent(const vk::Extent2D& panoramaExtent)
    {
        Assert(panoramaExtent.width / panoramaExtent.height == 2);

        const uint32_t height = panoramaExtent.height / 2;

        if (height < kMaxCubemapExtent.height)
        {
            return vk::Extent2D(height, height);
        }

        return kMaxCubemapExtent;
    }

    static Texture CreateCubemapTexture(const Texture& panoramaTexture)
    {
        const vk::Extent2D& panoramaExtent = VulkanHelpers::GetExtent2D(
                VulkanContext::imageManager->GetImageDescription(panoramaTexture.image).extent);

        const vk::Extent2D environmentExtent = Details::GetCubemapExtent(panoramaExtent);

        return VulkanContext::textureManager->CreateCubeTexture(panoramaTexture, environmentExtent);
    }
}

EnvironmentComponent EnvironmentHelpers::LoadEnvironment(const Filepath& panoramaPath)
{
    const Texture panoramaTexture = VulkanContext::textureManager->CreateTexture(panoramaPath);

    const Texture cubemapTexture = Details::CreateCubemapTexture(panoramaTexture);

    const Texture irradianceTexture = RenderContext::imageBasedLighting->GenerateIrradianceTexture(cubemapTexture);
    const Texture reflectionTexture = RenderContext::imageBasedLighting->GenerateReflectionTexture(cubemapTexture);

    VulkanContext::textureManager->DestroyTexture(panoramaTexture);

    return EnvironmentComponent{ cubemapTexture, irradianceTexture, reflectionTexture };
}

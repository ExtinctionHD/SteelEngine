#include "Engine/Scene/Components/EnvironmentComponent.hpp"

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
}

EnvironmentComponent EnvironmentHelpers::LoadEnvironment(const Filepath& panoramaPath)
{
    EASY_FUNCTION()

    const Texture panoramaTexture = TextureCache::GetTexture(panoramaPath);

    const Texture cubemapTexture = TextureCache::CreateCubeTexture(panoramaTexture.image);

    const Texture irradianceTexture = RenderContext::imageBasedLighting->GenerateIrradiance(cubemapTexture);
    const Texture reflectionTexture = RenderContext::imageBasedLighting->GenerateReflection(cubemapTexture);

    TextureCache::ReleaseTexture(panoramaPath, true);

    return EnvironmentComponent{ cubemapTexture, irradianceTexture, reflectionTexture };
}

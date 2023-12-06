#include "Engine/Scene/Components/EnvironmentComponent.hpp"

#include "Engine/Render/RenderContext.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Scene/ImageBasedLighting.hpp"

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

#include "Engine/Scene/Components/EnvironmentComponent.hpp"

#include "Engine/Render/RenderContext.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Scene/ImageBasedLighting.hpp"
#include "Utils/Logger.hpp"

EnvironmentComponent EnvironmentHelpers::LoadEnvironment(const Filepath& panoramaPath)
{
    Texture panoramaTexture;
    {
        LOG_SCOPE_TIMING << "Load environment panorama";

        panoramaTexture = TextureCache::GetTexture(panoramaPath);
    }

    Texture cubemapTexture;
    {
        LOG_SCOPE_TIMING << "Create environment cubemap";

        cubemapTexture = TextureCache::CreateCubeTexture(panoramaTexture.image);
    }

    Texture irradianceTexture;
    {
        LOG_SCOPE_TIMING << "Generate environment irradiance";

        irradianceTexture = RenderContext::imageBasedLighting->GenerateIrradiance(cubemapTexture);
    }

    Texture reflectionTexture;
    {
        LOG_SCOPE_TIMING << "Generate environment reflection";

        reflectionTexture = RenderContext::imageBasedLighting->GenerateReflection(cubemapTexture);
    }

    TextureCache::ReleaseTexture(panoramaPath, true);

    return EnvironmentComponent{ cubemapTexture, irradianceTexture, reflectionTexture };
}

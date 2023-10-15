#include "Engine/Scene/Components/EnvironmentComponent.hpp"

#include "Engine/Render/RenderContext.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Resources/ResourceHelpers.hpp"
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

    static CubeImage CreateCubemapImage(const BaseImage& panoramaImage)
    {
        EASY_FUNCTION()

        const ImageDescription description = VulkanContext::imageManager->GetImageDescription(panoramaImage.image);

        const vk::Extent2D& panoramaExtent = description.extent;

        const vk::Extent2D environmentExtent = Details::GetCubemapExtent(panoramaExtent);

        return VulkanContext::textureManager->CreateCubeTexture(panoramaImage, environmentExtent);
    }
}

EnvironmentComponent EnvironmentHelpers::LoadEnvironment(const Filepath& panoramaPath)
{
    EASY_FUNCTION()

    const BaseImage panoramaImage = VulkanContext::textureManager->CreateTexture(panoramaPath);

    const CubeImage cubemapImage = Details::CreateCubemapImage(panoramaImage);

    const CubeImage irradianceImage = RenderContext::imageBasedLighting->GenerateIrradianceImage(cubemapImage);
    const CubeImage reflectionImage = RenderContext::imageBasedLighting->GenerateReflectionImage(cubemapImage);

    ResourceHelpers::DestroyResource(panoramaImage);

    return EnvironmentComponent{ cubemapImage, irradianceImage, reflectionImage };
}

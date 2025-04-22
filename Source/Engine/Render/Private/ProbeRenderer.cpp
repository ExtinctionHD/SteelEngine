#include "Engine/Render/ProbeRenderer.hpp"

#include "Engine/Render/Vulkan/Resources/ImageHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/ResourceContext.hpp"

namespace Details
{
    constexpr vk::Format kProbeFormat = vk::Format::eR16G16B16A16Sfloat;

    constexpr vk::Extent2D kProbeExtent(32, 32);

    constexpr CameraComponent kCamera{
        .yFov = glm::radians(90.0f),
        .width = 1.0f,
        .height = 1.0f,
        .zNear = 0.01f,
        .zFar = 1000.0f
    };

    static BaseImage CreateProbeImage()
    {
        constexpr vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled;

        return ResourceContext::CreateCubeImage({
            .format = kProbeFormat,
            .extent = kProbeExtent,
            .mipLevelCount = 1,
            .usage = usage,
        });
    }
}

ProbeRenderer::ProbeRenderer(const Scene* scene_)
    : PathTracingStage(dummyContext)
{
    PathTracingStage::RegisterScene(scene_);
}

BaseImage ProbeRenderer::CaptureProbe(const glm::vec3&) const
{
    EASY_FUNCTION()

    const BaseImage probeImage = Details::CreateProbeImage();

    VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
        {
            {
                const ImageLayoutTransition layoutTransition{
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eGeneral,
                    PipelineBarrier{
                        SyncScope::kWaitForNone,
                        SyncScope::kRayTracingShaderWrite
                    }
                };

                ImageHelpers::TransitImageLayout(commandBuffer, probeImage.image,
                        ImageHelpers::kCubeColor, layoutTransition);
            }

            for (uint32_t faceIndex = 0; faceIndex < ImageHelpers::kCubeFaceCount; ++faceIndex)
            {
                PathTracingStage::Render(commandBuffer, faceIndex);
            }

            {
                const ImageLayoutTransition layoutTransition{
                    vk::ImageLayout::eGeneral,
                    vk::ImageLayout::eShaderReadOnlyOptimal,
                    PipelineBarrier{
                        SyncScope::kRayTracingShaderWrite,
                        SyncScope::kBlockNone
                    }
                };

                ImageHelpers::TransitImageLayout(commandBuffer, probeImage.image,
                        ImageHelpers::kCubeColor, layoutTransition);
            }
        });

    return probeImage;
}

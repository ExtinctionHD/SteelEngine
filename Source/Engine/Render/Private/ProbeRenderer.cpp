#include "Engine/Render/ProbeRenderer.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Resources/ImageHelpers.hpp"

namespace Details
{
    constexpr vk::Format kProbeFormat = vk::Format::eR16G16B16A16Sfloat;

    constexpr vk::Extent2D kProbeExtent(32, 32);

    constexpr CameraProjection kCameraProjection{
        .yFov = glm::radians(90.0f),
        .width = 1.0f,
        .height = 1.0f,
        .zNear = 0.01f,
        .zFar = 1000.0f
    };

    static CubeImage CreateProbeImage()
    {
        constexpr vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled;

        const CubeImageDescription description{
            .format = kProbeFormat,
            .extent = kProbeExtent,
            .mipLevelCount = 1,
            .usage = usage,
        };

        return VulkanContext::imageManager->CreateCubeImage(description, ImageCreateFlags::kNone);
    }

    static glm::vec3 GetCameraDirection(uint32_t faceIndex)
    {
        return ImageHelpers::kCubeFacesDirections[faceIndex];
    }

    static glm::vec3 GetCameraUp(uint32_t faceIndex)
    {
        const glm::vec3& direction = GetCameraDirection(faceIndex);

        if (direction.y == 0.0f)
        {
            return Direction::kUp;
        }

        return glm::cross(direction, Direction::kRight);
    }
}

ProbeRenderer::ProbeRenderer(const Scene* scene_)
{
    RegisterScene(scene_);

    cameraComponent.projection = Details::kCameraProjection;
    cameraComponent.projMatrix = CameraHelpers::ComputeProjMatrix(cameraComponent.projection);
}

CubeImage ProbeRenderer::CaptureProbe(const glm::vec3& position)
{
    EASY_FUNCTION()

    const CubeImage probeImage = Details::CreateProbeImage();

    cameraComponent.location.position = position;

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
                cameraComponent.location.up = Details::GetCameraUp(faceIndex);
                cameraComponent.location.direction = Details::GetCameraDirection(faceIndex);

                cameraComponent.viewMatrix = CameraHelpers::ComputeViewMatrix(cameraComponent.location);

                PathTracingRenderer::Render(commandBuffer, faceIndex);
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

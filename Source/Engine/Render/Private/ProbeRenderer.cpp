#include "Engine/Render/ProbeRenderer.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Resources/ImageHelpers.hpp"

namespace Details
{
    constexpr uint32_t kSampleCount = 16;

    constexpr vk::Format kProbeFormat = vk::Format::eR8G8B8A8Unorm;

    constexpr vk::Extent2D kProbeExtent(64, 64);

    constexpr Camera::Description kCameraDescription{
        .position = Vector3::kZero,
        .target = Direction::kForward,
        .up = Direction::kUp,
        .xFov = glm::radians(90.0f),
        .aspectRatio = 1.0f,
        .zNear = 0.01f,
        .zFar = 1000.0f
    };

    static vk::Image CreateProbeImage()
    {
        constexpr vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled;

        const ImageDescription imageDescription{
            ImageType::eCube, kProbeFormat,
            VulkanHelpers::GetExtent3D(kProbeExtent),
            1, ImageHelpers::kCubeFaceCount,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal, usage,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        };

        return VulkanContext::imageManager->CreateImage(imageDescription, ImageCreateFlags::kNone);
    }

    static glm::vec3 GetCameraDirection(uint32_t faceIndex)
    {
        return ImageHelpers::kCubeFacesDirections[faceIndex];
    }

    static glm::vec3 GetCameraUp(uint32_t faceIndex)
    {
        const glm::vec3& direction = ImageHelpers::kCubeFacesDirections[faceIndex];

        if (direction.y == 0.0f)
        {
            return Direction::kUp;
        }

        return glm::cross(direction, Direction::kRight);
    }
}

ProbeRenderer::ProbeRenderer(ScenePT* scene_, Environment* environment_)
    : Camera(Details::kCameraDescription)
    , PathTracer(scene_, this, environment_, Details::kSampleCount, Details::kProbeExtent)
{}

Texture ProbeRenderer::CaptureProbe(const glm::vec3& position)
{
    const vk::Image probeImage = Details::CreateProbeImage();

    const ImageHelpers::CubeFacesViews probeFacesViews
            = ImageHelpers::CreateCubeFacesViews(probeImage, 0);

    SetupRenderTargetsDescriptorSet(probeFacesViews);

    Camera::SetPosition(position);

    for (uint32_t faceIndex = 0; faceIndex < ImageHelpers::kCubeFaceCount; ++faceIndex)
    {
        Camera::SetDirection(Details::GetCameraDirection(faceIndex));
        Camera::SetUp(Details::GetCameraUp(faceIndex));

        Camera::UpdateViewMatrix();

        VulkanContext::device->ExecuteOneTimeCommands([&](vk::CommandBuffer commandBuffer)
            {
                if (faceIndex == 0)
                {
                    const ImageLayoutTransition layoutTransition{
                        vk::ImageLayout::eUndefined,
                        vk::ImageLayout::eGeneral,
                        PipelineBarrier{
                            SyncScope::kWaitForNone,
                            PathTracer::GetWriteSyncScope()
                        }
                    };

                    ImageHelpers::TransitImageLayout(commandBuffer, probeImage,
                            ImageHelpers::kCubeColor, layoutTransition);
                }

                PathTracer::Render(commandBuffer, faceIndex);

                if (faceIndex == ImageHelpers::kCubeFaceCount - 1)
                {
                    const ImageLayoutTransition layoutTransition{
                        vk::ImageLayout::eGeneral,
                        vk::ImageLayout::eShaderReadOnlyOptimal,
                        PipelineBarrier{
                            PathTracer::GetWriteSyncScope(),
                            SyncScope::kBlockNone
                        }
                    };

                    ImageHelpers::TransitImageLayout(commandBuffer, probeImage,
                            ImageHelpers::kCubeColor, layoutTransition);
                }
            });
    }

    for (const auto& view : probeFacesViews)
    {
        VulkanContext::imageManager->DestroyImageView(probeImage, view);
    }

    const vk::ImageView probeView = VulkanContext::imageManager->CreateView(
            probeImage, vk::ImageViewType::eCube, ImageHelpers::kCubeColor);

    return Texture{ probeImage, probeView };
}

void ProbeRenderer::SetupRenderTargetsDescriptorSet(const ImageHelpers::CubeFacesViews& probeFacesViews)
{
    if (!renderTargets.descriptorSet.values.empty())
    {
        VulkanContext::descriptorPool->FreeDescriptorSets(renderTargets.descriptorSet.values);
    }

    const DescriptorPool& descriptorPool = *VulkanContext::descriptorPool;

    renderTargets.descriptorSet.values = descriptorPool.AllocateDescriptorSets(
            Repeat(renderTargets.descriptorSet.layout, probeFacesViews.size()));

    for (size_t i = 0; i < probeFacesViews.size(); ++i)
    {
        const DescriptorData descriptorData = DescriptorHelpers::GetStorageData(probeFacesViews[i]);

        descriptorPool.UpdateDescriptorSet(renderTargets.descriptorSet.values[i], { descriptorData, }, 0);
    }
}

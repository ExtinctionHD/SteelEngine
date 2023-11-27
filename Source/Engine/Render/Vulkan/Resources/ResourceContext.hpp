#pragma once

#include "Engine/Render/FrameLoop.hpp"
#include "Engine/Render/RenderContext.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"
#include "Engine/Render/Vulkan/Resources/ImageManager.hpp"
#include "Engine/Render/Vulkan/Resources/BufferManager.hpp"
#include "Engine/Render/Vulkan/Resources/AccelerationStructureManager.hpp"

class ResourceContext
{
public:
    static void Create();
    static void Destroy();

    static const ImageDescription& GetImageDescription(vk::Image image);

    static const BufferDescription& GetBufferDescription(vk::Buffer buffer);

    static vk::Image CreateImage(const ImageDescription& description);

    static vk::ImageView CreateImageView(const ImageViewDescription& description);

    static CubeFaceViews CreateImageCubeFaceViews(const vk::Image image);

    static BaseImage CreateBaseImage(const ImageDescription& description);

    static BaseImage CreateCubeImage(const CubeImageDescription& description);

    static vk::Buffer CreateBuffer(const BufferDescription& description);

    static vk::AccelerationStructureKHR GenerateBlas(const BlasGeometryData& data);

    static vk::AccelerationStructureKHR CreateTlas(const TlasInstances& instances);

    static void UpdateImage(vk::CommandBuffer commandBuffer,
            vk::Image image, const ImageUpdateRegions& updateRegions);

    static void UpdateBuffer(vk::CommandBuffer commandBuffer,
            vk::Buffer buffer, const BufferUpdate& update);

    static void ReadBuffer(vk::CommandBuffer commandBuffer,
            vk::Buffer buffer, const BufferReader& reader);

    static void BuildTlas(vk::CommandBuffer commandBuffer,
            vk::AccelerationStructureKHR tlas, const TlasInstances& instances);

    template <class T>
    static void DestroyResource(T resource)
    {
        static_assert(
            std::is_same_v<T, BaseImage> ||
            std::is_same_v<T, vk::Image> ||
            std::is_same_v<T, vk::ImageView> ||
            std::is_same_v<T, vk::Buffer> ||
            std::is_same_v<T, vk::Sampler> ||
            std::is_same_v<T, vk::AccelerationStructureKHR>);

        if constexpr (std::is_same_v<T, BaseImage>)
        {
            imageManager->DestroyImage(resource.image);
        }
        if constexpr (std::is_same_v<T, vk::Image>)
        {
            imageManager->DestroyImage(resource);
        }
        if constexpr (std::is_same_v<T, vk::ImageView>)
        {
            imageManager->DestroyView(resource);
        }
        if constexpr (std::is_same_v<T, vk::Buffer>)
        {
            bufferManager->DestroyBuffer(resource);
        }
        if constexpr (std::is_same_v<T, vk::Sampler>)
        {
            VulkanContext::device->Get().destroySampler(resource);
        }
        if constexpr (std::is_same_v<T, vk::AccelerationStructureKHR>)
        {
            accelerationStructureManager->DestroyAccelerationStructure(resource);
        }
    }

    template <class T>
    static void DestroyResourceDelayed(T resource)
    {
        RenderContext::frameLoop->Destroy([resource]()
            {
                DestroyResource(resource);
            });
    }

private:
    static std::unique_ptr<ImageManager> imageManager;
    static std::unique_ptr<BufferManager> bufferManager;
    static std::unique_ptr<AccelerationStructureManager> accelerationStructureManager;
};

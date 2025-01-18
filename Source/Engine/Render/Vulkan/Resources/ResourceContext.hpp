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

    static vk::AccelerationStructureKHR CreateTlas(uint32_t instanceCount);

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
        static_assert(std::is_base_of_v<BaseImage, T>
            || std::is_base_of_v<vk::Image, T>
            || std::is_base_of_v<vk::ImageView, T>
            || std::is_base_of_v<vk::Buffer, T>
            || std::is_base_of_v<vk::Sampler, T>
            || std::is_base_of_v<vk::AccelerationStructureKHR, T>);

        if constexpr (std::is_base_of_v<BaseImage, T>)
        {
            // TODO remove this branch if BaseImage is inherited from vk::Image
            imageManager->DestroyImage(resource.image);
        }
        if constexpr (std::is_base_of_v<vk::Image, T>)
        {
            imageManager->DestroyImage(resource);
        }
        if constexpr (std::is_base_of_v<vk::ImageView, T>)
        {
            imageManager->DestroyView(resource);
        }
        if constexpr (std::is_base_of_v<vk::Buffer, T>)
        {
            bufferManager->DestroyBuffer(resource);
        }
        if constexpr (std::is_base_of_v<vk::Sampler, T>)
        {
            VulkanContext::device->Get().destroySampler(resource);
        }
        if constexpr (std::is_base_of_v<vk::AccelerationStructureKHR, T>)
        {
            accelerationStructureManager->DestroyAccelerationStructure(resource);
        }
    }

    template <class T>
    static void DestroyResourceSafe(T resource)
    {
        RenderContext::frameLoop->DestroyResource([resource]()
            {
                DestroyResource(resource);
            });
    }

private:
    static std::unique_ptr<ImageManager> imageManager;
    static std::unique_ptr<BufferManager> bufferManager;
    static std::unique_ptr<AccelerationStructureManager> accelerationStructureManager;
};

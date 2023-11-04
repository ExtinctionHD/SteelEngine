#pragma once

#include "Engine/Render/FrameLoop.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"

struct Texture;

namespace ResourceHelpers
{
    template <class T>
    void DestroyResource(T resource)
    {
        static_assert(std::is_same_v<T, Texture> || std::is_same_v<T, vk::Sampler> || std::is_same_v<T, vk::Image> || std::is_same_v<T, vk::Buffer> || std::is_same_v<T, vk::AccelerationStructureKHR>);

        if constexpr (std::is_same_v<T, Texture>)
        {
            VulkanContext::textureManager->DestroyTexture(resource);
        }
        if constexpr (std::is_same_v<T, vk::Sampler>)
        {
            VulkanContext::textureManager->DestroySampler(resource);
        }
        if constexpr (std::is_same_v<T, vk::Image>)
        {
            VulkanContext::imageManager->DestroyImage(resource);
        }
        if constexpr (std::is_same_v<T, vk::Buffer>)
        {
            VulkanContext::bufferManager->DestroyBuffer(resource);
        }
        if constexpr (std::is_same_v<T, vk::AccelerationStructureKHR>)
        {
            VulkanContext::accelerationStructureManager->DestroyAccelerationStructure(resource);
        }
    }

    template <class T>
    void DestroyResourceDelayed(T resource)
    {
        RenderContext::frameLoop->DestroyResource([resource]()
                { DestroyResource(resource); });
    }
}

#pragma once

class VulkanDevice;

namespace VulkanHelpers
{
    const vk::ComponentMapping kComponentMappingRgba(
            vk::ComponentSwizzle::eR,
            vk::ComponentSwizzle::eG,
            vk::ComponentSwizzle::eB,
            vk::ComponentSwizzle::eA);

    const vk::ImageSubresourceRange kSubresourceRangeDefault(
            vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

    bool IsDepthFormat(vk::Format format);

    vk::DeviceMemory AllocateDeviceMemory(const VulkanDevice &device,
            vk::MemoryRequirements requirements, vk::MemoryPropertyFlags properties);
}

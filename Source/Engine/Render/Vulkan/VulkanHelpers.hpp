#pragma once

class Device;

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

    vk::DeviceMemory AllocateDeviceMemory(const Device &device,
            vk::MemoryRequirements requirements, vk::MemoryPropertyFlags properties);

    void CopyToDeviceMemory(const Device &device, const uint8_t *src,
            vk::DeviceMemory memory, uint32_t memoryOffset, uint32_t size);
}

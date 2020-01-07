#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

#include "Engine/Render/Vulkan/VulkanDevice.hpp"

#include "Utils/Assert.hpp"

bool VulkanHelpers::IsDepthFormat(vk::Format format)
{
    switch (format)
    {
    case vk::Format::eD16Unorm:
    case vk::Format::eX8D24UnormPack32:
    case vk::Format::eD32Sfloat:
    case vk::Format::eD16UnormS8Uint:
    case vk::Format::eD24UnormS8Uint:
    case vk::Format::eD32SfloatS8Uint:
        return true;
    default:
        return false;
    }
}

vk::DeviceMemory VulkanHelpers::AllocateDeviceMemory(const VulkanDevice &device,
        vk::MemoryRequirements requirements, vk::MemoryPropertyFlags properties)
{
    const uint32_t memoryTypeIndex = device.GetMemoryTypeIndex(requirements.memoryTypeBits, properties);
    const vk::MemoryAllocateInfo allocateInfo(requirements.size, memoryTypeIndex);

    const auto [result, memory] = device.Get().allocateMemory(allocateInfo);
    Assert(result == vk::Result::eSuccess);

    return memory;
}
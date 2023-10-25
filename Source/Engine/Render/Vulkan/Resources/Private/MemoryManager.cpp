#define VMA_IMPLEMENTATION

#include "Engine/Render/Vulkan/Resources/MemoryManager.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

namespace Details
{
    static VmaAllocationCreateInfo GetAllocationCreateInfo(vk::MemoryPropertyFlags memoryProperties)
    {
        VmaAllocationCreateInfo allocationCreateInfo = {};
        allocationCreateInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(memoryProperties);

        return allocationCreateInfo;
    }
}

bool MemoryBlock::operator==(const MemoryBlock& other) const
{
    return memory == other.memory && offset == other.offset && size == other.size;
}

bool MemoryBlock::operator<(const MemoryBlock& other) const
{
    if (memory == other.memory)
    {
        if (offset == other.offset)
        {
            return size < other.size;
        }

        return offset < other.offset;
    }

    return memory < other.memory;
}

MemoryManager::MemoryManager()
{
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.instance = VulkanContext::instance->Get();
    allocatorInfo.physicalDevice = VulkanContext::device->GetPhysicalDevice();
    allocatorInfo.device = VulkanContext::device->Get();
    allocatorInfo.flags = VmaAllocatorCreateFlagBits::VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    vmaCreateAllocator(&allocatorInfo, &allocator);
}

MemoryManager::~MemoryManager()
{
    vmaDestroyAllocator(allocator);
}

vk::Buffer MemoryManager::CreateBuffer(
    const vk::BufferCreateInfo& createInfo, vk::MemoryPropertyFlags memoryProperties)
{
    const VmaAllocationCreateInfo allocationCreateInfo = Details::GetAllocationCreateInfo(memoryProperties);

    VkBuffer buffer;
    VmaAllocation allocation;

    const VkResult result = vmaCreateBuffer(allocator, &createInfo.operator VkBufferCreateInfo const&(), &allocationCreateInfo, &buffer, &allocation, nullptr);

    Assert(result == VK_SUCCESS);

    bufferAllocations.emplace(buffer, allocation);

    return buffer;
}

vk::Buffer MemoryManager::CreateBuffer(const vk::BufferCreateInfo& createInfo, vk::MemoryPropertyFlags memoryProperties, vk::DeviceSize minMemoryAlignment)
{
    auto [result, buffer] = VulkanContext::device->Get().createBuffer(createInfo);
    Assert(result == vk::Result::eSuccess);

    vk::MemoryRequirements memoryRequirements
        = VulkanContext::device->Get().getBufferMemoryRequirements(buffer);
    memoryRequirements.alignment = std::max(memoryRequirements.alignment, minMemoryAlignment);

    const VmaAllocationCreateInfo allocationCreateInfo = Details::GetAllocationCreateInfo(memoryProperties);

    VmaAllocation allocation;
    VmaAllocationInfo allocationInfo;

    vmaAllocateMemory(allocator, &memoryRequirements.operator VkMemoryRequirements const&(), &allocationCreateInfo, &allocation, &allocationInfo);

    result = VulkanContext::device->Get().bindBufferMemory(
        buffer, allocationInfo.deviceMemory, allocationInfo.offset);
    Assert(result == vk::Result::eSuccess);

    bufferAllocations.emplace(buffer, allocation);

    return buffer;
}

void MemoryManager::DestroyBuffer(vk::Buffer buffer)
{
    const auto it = bufferAllocations.find(buffer);
    Assert(it != bufferAllocations.end());

    vmaDestroyBuffer(allocator, buffer, it->second);

    bufferAllocations.erase(it);
}

vk::Image MemoryManager::CreateImage(
    const vk::ImageCreateInfo& createInfo, vk::MemoryPropertyFlags memoryProperties)
{
    const VmaAllocationCreateInfo allocationCreateInfo = Details::GetAllocationCreateInfo(memoryProperties);

    VkImage image;
    VmaAllocation allocation;

    const VkResult result = vmaCreateImage(allocator, &createInfo.operator VkImageCreateInfo const&(), &allocationCreateInfo, &image, &allocation, nullptr);

    Assert(result == VK_SUCCESS);

    imageAllocations.emplace(image, allocation);

    return image;
}

void MemoryManager::DestroyImage(vk::Image image)
{
    const auto it = imageAllocations.find(image);
    Assert(it != imageAllocations.end());

    vmaDestroyImage(allocator, image, it->second);

    imageAllocations.erase(it);
}

MemoryBlock MemoryManager::GetBufferMemoryBlock(vk::Buffer buffer) const
{
    return GetMemoryBlock(buffer, bufferAllocations);
}

MemoryBlock MemoryManager::GetImageMemoryBlock(vk::Image image) const
{
    return GetMemoryBlock(image, imageAllocations);
}

MemoryBlock MemoryManager::GetAccelerationStructureMemoryBlock(
    vk::AccelerationStructureKHR accelerationStructure) const
{
    return GetMemoryBlock(accelerationStructure, accelerationStructureAllocations);
}

ByteAccess MemoryManager::MapMemory(const MemoryBlock& memoryBlock) const
{
    void* mappedMemory = nullptr;

    const vk::Result result = VulkanContext::device->Get().mapMemory(
        memoryBlock.memory, memoryBlock.offset, memoryBlock.size, vk::MemoryMapFlags(), &mappedMemory);

    Assert(result == vk::Result::eSuccess);

    return ByteAccess(static_cast<uint8_t*>(mappedMemory), memoryBlock.size);
}

void MemoryManager::UnmapMemory(const MemoryBlock& memoryBlock) const
{
    VulkanContext::device->Get().unmapMemory(memoryBlock.memory);
}

MemoryBlock MemoryManager::AllocateMemory(
    const vk::MemoryRequirements& memoryRequirements, vk::MemoryPropertyFlags memoryProperties)
{
    const VmaAllocationCreateInfo allocationCreateInfo = Details::GetAllocationCreateInfo(memoryProperties);

    VmaAllocation allocation;
    VmaAllocationInfo allocationInfo;

    vmaAllocateMemory(allocator, &memoryRequirements.operator VkMemoryRequirements const&(), &allocationCreateInfo, &allocation, &allocationInfo);

    const MemoryBlock memoryBlock{ allocationInfo.deviceMemory, allocationInfo.offset, allocationInfo.size };

    memoryAllocations.emplace(memoryBlock, allocation);

    return memoryBlock;
}

void MemoryManager::FreeMemory(const MemoryBlock& memoryBlock)
{
    const auto it = memoryAllocations.find(memoryBlock);
    Assert(it != memoryAllocations.end());

    vmaFreeMemory(allocator, it->second);

    memoryAllocations.erase(it);
}

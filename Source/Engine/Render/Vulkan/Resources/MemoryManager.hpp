#pragma once

#pragma warning(push, 0)
#include <src/vk_mem_alloc.h>
#pragma warning(pop)

#include "Engine/Render/Vulkan/Resources/MemoryBlock.hpp"

#include "Utils/DataHelpers.hpp"
#include "Utils/Assert.hpp"

class MemoryManager
{
public:
    MemoryManager();
    ~MemoryManager();

    MemoryBlock AllocateMemory(const vk::MemoryRequirements& requirements, vk::MemoryPropertyFlags properties);
    void FreeMemory(const MemoryBlock& memoryBlock);

    void CopyDataToMemory(const ByteView& data, const MemoryBlock& memoryBlock) const;

    vk::Buffer CreateBuffer(const vk::BufferCreateInfo& createInfo, vk::MemoryPropertyFlags memoryProperties);
    void DestroyBuffer(vk::Buffer buffer);

    vk::Image CreateImage(const vk::ImageCreateInfo& createInfo, vk::MemoryPropertyFlags memoryProperties);
    void DestroyImage(vk::Image image);

    vk::AccelerationStructureKHR CreateAccelerationStructure(const vk::AccelerationStructureCreateInfoKHR& createInfo);
    void DestroyAccelerationStructure(vk::AccelerationStructureKHR accelerationStructure);

    MemoryBlock GetBufferMemoryBlock(vk::Buffer buffer) const;

    MemoryBlock GetImageMemoryBlock(vk::Image image) const;

    MemoryBlock GetAccelerationStructureMemoryBlock(vk::AccelerationStructureKHR accelerationStructure) const;

private:
    VmaAllocator allocator = nullptr;

    std::unordered_map<MemoryBlock, VmaAllocation> memoryAllocations;

    std::map<vk::Buffer, VmaAllocation> bufferAllocations;
    std::map<vk::Image, VmaAllocation> imageAllocations;
    std::map<vk::AccelerationStructureKHR, VmaAllocation> accelerationStructureAllocations;

    template <class T>
    MemoryBlock GetObjectMemoryBlock(T object, std::map<T, VmaAllocation> allocations) const;
};

template <class T>
MemoryBlock MemoryManager::GetObjectMemoryBlock(T object, std::map<T, VmaAllocation> allocations) const
{
    const auto it = allocations.find(object);
    Assert(it != allocations.end());

    VmaAllocationInfo allocationInfo;
    vmaGetAllocationInfo(allocator, it->second, &allocationInfo);

    return MemoryBlock{ allocationInfo.deviceMemory, allocationInfo.offset, allocationInfo.size };
}

#pragma once

#pragma warning(push, 0)
#include <src/vk_mem_alloc.h>
#pragma warning(pop)

#include "Utils/DataHelpers.hpp"
#include "Utils/Assert.hpp"

struct MemoryBlock
{
    vk::DeviceMemory memory;
    vk::DeviceSize offset;
    vk::DeviceSize size;

    bool operator==(const MemoryBlock& other) const;

    bool operator<(const MemoryBlock& other) const;
};

class MemoryManager
{
public:
    MemoryManager();
    ~MemoryManager();

    vk::Buffer CreateBuffer(const vk::BufferCreateInfo& createInfo, vk::MemoryPropertyFlags memoryProperties);

    vk::Buffer CreateBuffer(const vk::BufferCreateInfo& createInfo, vk::MemoryPropertyFlags memoryProperties,
            vk::DeviceSize minMemoryAlignment);

    void DestroyBuffer(vk::Buffer buffer);

    vk::Image CreateImage(const vk::ImageCreateInfo& createInfo, vk::MemoryPropertyFlags memoryProperties);

    void DestroyImage(vk::Image image);

    MemoryBlock GetBufferMemoryBlock(vk::Buffer buffer) const;

    MemoryBlock GetImageMemoryBlock(vk::Image image) const;

    MemoryBlock GetAccelerationStructureMemoryBlock(vk::AccelerationStructureKHR accelerationStructure) const;

    ByteAccess MapMemory(const MemoryBlock& memoryBlock) const;

    void UnmapMemory(const MemoryBlock& memoryBlock) const;

private:
    VmaAllocator allocator = nullptr;

    std::map<MemoryBlock, VmaAllocation> memoryAllocations;

    std::map<vk::Buffer, VmaAllocation> bufferAllocations;
    std::map<vk::Image, VmaAllocation> imageAllocations;
    std::map<vk::AccelerationStructureKHR, VmaAllocation> accelerationStructureAllocations;

    MemoryBlock AllocateMemory(const vk::MemoryRequirements& requirements, vk::MemoryPropertyFlags properties);

    template <class T>
    MemoryBlock GetMemoryBlock(T object, std::map<T, VmaAllocation> allocations) const;

    void FreeMemory(const MemoryBlock& memoryBlock);
};

template <class T>
MemoryBlock MemoryManager::GetMemoryBlock(T object, std::map<T, VmaAllocation> allocations) const
{
    const auto it = allocations.find(object);
    Assert(it != allocations.end());

    VmaAllocationInfo allocationInfo;
    vmaGetAllocationInfo(allocator, it->second, &allocationInfo);

    return MemoryBlock{ allocationInfo.deviceMemory, allocationInfo.offset, allocationInfo.size };
}

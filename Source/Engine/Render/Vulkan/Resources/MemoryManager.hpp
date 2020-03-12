#pragma once

#pragma warning(push, 0)
#include <src/vk_mem_alloc.h>
#pragma warning(pop)

#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Resources/MemoryBlock.hpp"

#include "Utils/DataHelpers.hpp"
#include "Utils/Assert.hpp"

class MemoryManager
{
public:
    MemoryManager(std::shared_ptr<Device> device_);
    ~MemoryManager();

    MemoryBlock AllocateMemory(const vk::MemoryRequirements &requirements, vk::MemoryPropertyFlags properties);
    void FreeMemory(const MemoryBlock &memoryBlock);

    void CopyDataToMemory(const ByteView &data, const MemoryBlock &memoryBlock) const;

    vk::Buffer CreateBuffer(const vk::BufferCreateInfo &createInfo, vk::MemoryPropertyFlags memoryProperties);
    void DestroyBuffer(vk::Buffer buffer);

    vk::Image CreateImage(const vk::ImageCreateInfo &createInfo, vk::MemoryPropertyFlags memoryProperties);
    void DestroyImage(vk::Image image);

    vk::AccelerationStructureNV CreateAccelerationStructure(const vk::AccelerationStructureCreateInfoNV &createInfo);
    void DestroyAccelerationStructure(vk::AccelerationStructureNV accelerationStructure);

    MemoryBlock GetBufferMemoryBlock(vk::Buffer buffer) const;

    MemoryBlock GetImageMemoryBlock(vk::Image image) const;

    MemoryBlock GetAccelerationStructureMemoryBlock(vk::AccelerationStructureNV accelerationStructure) const;

private:
    std::shared_ptr<Device> device;

    VmaAllocator allocator = nullptr;

    std::unordered_map<MemoryBlock, VmaAllocation> memoryAllocations;

    std::map<vk::Buffer, VmaAllocation> bufferAllocations;
    std::map<vk::Image, VmaAllocation> imageAllocations;
    std::map<vk::AccelerationStructureNV, VmaAllocation> accelerationStructureAllocations;

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

    return { allocationInfo.deviceMemory, allocationInfo.offset, allocationInfo.size };
}

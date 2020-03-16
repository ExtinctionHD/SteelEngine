#define VMA_IMPLEMENTATION

#include "Engine/Render/Vulkan/Resources/MemoryManager.hpp"

namespace SMemoryManager
{
    VmaAllocationCreateInfo GetAllocationCreateInfo(vk::MemoryPropertyFlags memoryProperties)
    {
        VmaAllocationCreateInfo allocationCreateInfo = {};
        allocationCreateInfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(memoryProperties);

        return allocationCreateInfo;
    }

    VkMemoryRequirements GetAccelerationStructureMemoryRequirements(vk::Device device,
            vk::AccelerationStructureNV accelerationStructure)
    {
        const vk::AccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo(
                vk::AccelerationStructureMemoryRequirementsTypeNV::eObject, accelerationStructure);
        const vk::MemoryRequirements2 memoryRequirements2
                = device.getAccelerationStructureMemoryRequirementsNV(memoryRequirementsInfo);
        const VkMemoryRequirements &memoryRequirements = memoryRequirements2.memoryRequirements;

        return memoryRequirements;
    }
}

MemoryManager::MemoryManager(std::shared_ptr<Device> device_)
    : device(device_)
{
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = device->GetPhysicalDevice();
    allocatorInfo.device = device->Get();

    vmaCreateAllocator(&allocatorInfo, &allocator);
}

MemoryManager::~MemoryManager()
{
    vmaDestroyAllocator(allocator);
}

MemoryBlock MemoryManager::AllocateMemory(const vk::MemoryRequirements &memoryRequirements,
        vk::MemoryPropertyFlags memoryProperties)
{
    const VmaAllocationCreateInfo allocationCreateInfo = SMemoryManager::GetAllocationCreateInfo(memoryProperties);

    VmaAllocation allocation;
    VmaAllocationInfo allocationInfo;

    vmaAllocateMemory(allocator, &memoryRequirements.operator struct VkMemoryRequirements const&(),
            &allocationCreateInfo, &allocation, &allocationInfo);

    const MemoryBlock memoryBlock{
        allocationInfo.deviceMemory,
        allocationInfo.offset,
        allocationInfo.size
    };

    memoryAllocations.emplace(memoryBlock, allocation);

    return memoryBlock;
}

void MemoryManager::FreeMemory(const MemoryBlock &memoryBlock)
{
    const auto it = memoryAllocations.find(memoryBlock);
    Assert(it != memoryAllocations.end());

    vmaFreeMemory(allocator, it->second);

    memoryAllocations.erase(it);
}

void MemoryManager::CopyDataToMemory(const ByteView &data, const MemoryBlock &memoryBlock) const
{
    Assert(data.size <= memoryBlock.size);

    void *mappedMemory = nullptr;

    const vk::Result result = device->Get().mapMemory(memoryBlock.memory,
            memoryBlock.offset, data.size, vk::MemoryMapFlags(), &mappedMemory);

    Assert(result == vk::Result::eSuccess);

    std::copy(data.data, data.data + data.size, reinterpret_cast<uint8_t *>(mappedMemory));

    device->Get().unmapMemory(memoryBlock.memory);
}

vk::Buffer MemoryManager::CreateBuffer(const vk::BufferCreateInfo &createInfo, vk::MemoryPropertyFlags memoryProperties)
{
    const VmaAllocationCreateInfo allocationCreateInfo = SMemoryManager::GetAllocationCreateInfo(memoryProperties);

    VkBuffer buffer;
    VmaAllocation allocation;

    const VkResult result = vmaCreateBuffer(allocator, &createInfo.operator struct VkBufferCreateInfo const&(),
            &allocationCreateInfo, &buffer, &allocation, nullptr);

    Assert(result == VK_SUCCESS);

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

vk::Image MemoryManager::CreateImage(const vk::ImageCreateInfo &createInfo, vk::MemoryPropertyFlags memoryProperties)
{
    const VmaAllocationCreateInfo allocationCreateInfo = SMemoryManager::GetAllocationCreateInfo(memoryProperties);

    VkImage image;
    VmaAllocation allocation;

    const VkResult result = vmaCreateImage(allocator, &createInfo.operator struct VkImageCreateInfo const &(),
            &allocationCreateInfo, &image, &allocation, nullptr);

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

vk::AccelerationStructureNV MemoryManager::CreateAccelerationStructure(
        const vk::AccelerationStructureCreateInfoNV &createInfo)
{
    const auto [result, accelerationStructure] = device->Get().createAccelerationStructureNV(createInfo);
    Assert(result == vk::Result::eSuccess);

    const VmaAllocationCreateInfo allocationCreateInfo = SMemoryManager::GetAllocationCreateInfo(
            vk::MemoryPropertyFlagBits::eDeviceLocal);

    const VkMemoryRequirements memoryRequirements = SMemoryManager::GetAccelerationStructureMemoryRequirements(
            device->Get(), accelerationStructure);

    VmaAllocation allocation;
    VmaAllocationInfo allocationInfo;

    const VkResult allocateResult = vmaAllocateMemory(allocator, &memoryRequirements,
            &allocationCreateInfo, &allocation, &allocationInfo);

    Assert(allocateResult == VK_SUCCESS);

    const vk::BindAccelerationStructureMemoryInfoNV bindInfo(accelerationStructure,
            allocationInfo.deviceMemory, allocationInfo.offset, 0, nullptr);

    const vk::Result bindResult = device->Get().bindAccelerationStructureMemoryNV(bindInfo);
    Assert(bindResult == vk::Result::eSuccess);

    accelerationStructureAllocations.emplace(accelerationStructure, allocation);

    return accelerationStructure;
}

void MemoryManager::DestroyAccelerationStructure(vk::AccelerationStructureNV accelerationStructure)
{
    const auto it = accelerationStructureAllocations.find(accelerationStructure);
    Assert(it != accelerationStructureAllocations.end());

    device->Get().destroyAccelerationStructureNV(accelerationStructure);
    vmaFreeMemory(allocator, it->second);

    accelerationStructureAllocations.erase(it);
}

MemoryBlock MemoryManager::GetBufferMemoryBlock(vk::Buffer buffer) const
{
    return GetObjectMemoryBlock(buffer, bufferAllocations);
}

MemoryBlock MemoryManager::GetImageMemoryBlock(vk::Image image) const
{
    return GetObjectMemoryBlock(image, imageAllocations);
}

MemoryBlock MemoryManager::GetAccelerationStructureMemoryBlock(vk::AccelerationStructureNV accelerationStructure) const
{
    return GetObjectMemoryBlock(accelerationStructure, accelerationStructureAllocations);
}

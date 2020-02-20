#include "Engine/Render/Vulkan/Resources/BufferManager.hpp"

#include "Engine/Render/Vulkan/Resources/ResourcesHelpers.hpp"
#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

#include "Utils/Helpers.hpp"

namespace SBufferManager
{
    vk::Buffer CreateBuffer(const Device &device, const BufferDescription &description)
    {
        const vk::BufferCreateInfo createInfo({}, description.size, description.usage,
                vk::SharingMode::eExclusive, 0, &device.GetQueueProperties().graphicsFamilyIndex);

        auto [result, buffer] = device.Get().createBuffer(createInfo);
        Assert(result == vk::Result::eSuccess);

        return buffer;
    }

    vk::DeviceMemory AllocateBufferMemory(const Device &device, vk::Buffer buffer,
            vk::MemoryPropertyFlags memoryProperties)
    {
        const vk::MemoryRequirements memoryRequirements = device.Get().getBufferMemoryRequirements(buffer);
        const vk::DeviceMemory memory = VulkanHelpers::AllocateDeviceMemory(device,
                memoryRequirements, memoryProperties);

        const vk::Result result = device.Get().bindBufferMemory(buffer, memory, 0);
        Assert(result == vk::Result::eSuccess);

        return memory;
    }
}

BufferManager::BufferManager(std::shared_ptr<Device> aDevice, std::shared_ptr<ResourceUpdateSystem> aUpdateSystem)
    : device(aDevice)
    , updateSystem(aUpdateSystem)
{}

BufferManager::~BufferManager()
{
    for (auto &[buffer, memory] : bufferStorage)
    {
        device->Get().destroyBuffer(buffer->buffer);
        device->Get().freeMemory(memory);
        delete buffer;
    }
}

BufferHandle BufferManager::CreateBuffer(const BufferDescription &description)
{
    Buffer *buffer = new Buffer();
    buffer->state = eResourceState::kUpdated;
    buffer->description = description;
    buffer->rawData = new uint8_t[description.size];
    buffer->buffer = SBufferManager::CreateBuffer(GetRef(device), description);

    const vk::DeviceMemory memory = SBufferManager::AllocateBufferMemory(GetRef(device),
            buffer->buffer, description.memoryProperties);

    bufferStorage.emplace_back(buffer, memory);

    return buffer;
}

void BufferManager::UpdateMarkedBuffers()
{
    std::vector<vk::MappedMemoryRange> memoryRanges;

    for (auto &[buffer, memory] : bufferStorage)
    {
        if (buffer->state == eResourceState::kMarkedForUpdate)
        {
            const vk::MemoryPropertyFlags memoryProperties = buffer->description.memoryProperties;
            const vk::DeviceSize size = buffer->description.size;

            if (memoryProperties & vk::MemoryPropertyFlagBits::eHostVisible)
            {
                VulkanHelpers::CopyToDeviceMemory(GetRef(device),
                        buffer->AccessData<uint8_t>().data, memory, 0, size);

                if (!(memoryProperties & vk::MemoryPropertyFlagBits::eHostCoherent))
                {
                    memoryRanges.emplace_back(memory, 0, size);
                }

                buffer->state = eResourceState::kUpdated;
            }
            else
            {
                updateSystem->EnqueueBufferUpdate(buffer);
            }
        }
    }

    if (!memoryRanges.empty())
    {
        device->Get().flushMappedMemoryRanges(
                static_cast<uint32_t>(memoryRanges.size()), memoryRanges.data());
    }
}

void BufferManager::DestroyBuffer(BufferHandle handle)
{
    Assert(handle != nullptr && handle->state != eResourceState::kUninitialized);

    const auto it = ResourcesHelpers::FindByHandle(handle, bufferStorage);
    auto &[buffer, memory] = *it;

    device->Get().destroyBuffer(buffer->buffer);
    device->Get().freeMemory(memory);
    delete buffer;

    bufferStorage.erase(it);
}

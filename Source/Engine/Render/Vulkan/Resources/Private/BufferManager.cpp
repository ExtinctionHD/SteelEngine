#include "Engine/Render/Vulkan/Resources/BufferManager.hpp"

#include "Engine/Render/Vulkan/Resources/ResourcesHelpers.hpp"
#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

#include "Utils/Helpers.hpp"

namespace SBufferManager
{
    vk::BufferCreateInfo GetBufferCreateInfo(const Device &device, const BufferDescription &description)
    {
        const vk::BufferCreateInfo createInfo({}, description.size, description.usage,
                vk::SharingMode::eExclusive, 0, &device.GetQueueProperties().graphicsFamilyIndex);

        return createInfo;
    }
}

BufferManager::BufferManager(std::shared_ptr<Device> aDevice, std::shared_ptr<MemoryManager> aMemoryManager,
        std::shared_ptr<ResourceUpdateSystem> aUpdateSystem)
    : device(aDevice)
    , memoryManager(aMemoryManager)
    , updateSystem(aUpdateSystem)
{}

BufferManager::~BufferManager()
{
    for (const auto &buffer : buffers)
    {
        memoryManager->DestroyBuffer(buffer->buffer);
        delete buffer;
    }
}

BufferHandle BufferManager::CreateBuffer(const BufferDescription &description)
{
    const vk::BufferCreateInfo createInfo = SBufferManager::GetBufferCreateInfo(GetRef(device), description);

    Buffer *buffer = new Buffer();
    buffer->state = eResourceState::kUpdated;
    buffer->description = description;
    buffer->rawData = new uint8_t[description.size];
    buffer->buffer = memoryManager->CreateBuffer(createInfo, description.memoryProperties);

    buffers.emplace_back(buffer);

    return buffer;
}

void BufferManager::EnqueueMarkedBuffersForUpdate()
{
    std::vector<vk::MappedMemoryRange> memoryRanges;

    for (const auto &buffer : buffers)
    {
        if (buffer->state == eResourceState::kMarkedForUpdate)
        {
            const vk::MemoryPropertyFlags memoryProperties = buffer->description.memoryProperties;
            const vk::DeviceSize size = buffer->description.size;

            if (memoryProperties & vk::MemoryPropertyFlagBits::eHostVisible)
            {
                // TODO: add function to memory manager
                Assert(false);

                buffer->state = eResourceState::kUpdated;
            }
            else
            {
                updateSystem->EnqueueBufferUpdate(buffer);
            }
        }
    }
}

void BufferManager::DestroyBuffer(BufferHandle handle)
{
    Assert(handle != nullptr && handle->state != eResourceState::kUninitialized);

    const auto it = std::find(buffers.begin(), buffers.end(), handle);
    Assert(it != buffers.end());

    memoryManager->DestroyBuffer(handle->buffer);
    delete handle;

    buffers.erase(it);
}

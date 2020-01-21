#include "Engine/Render/Vulkan/Resources/BufferManager.hpp"

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

#include "Utils/Helpers.hpp"

namespace SBufferManager
{
    bool RequireTransferSystem(vk::MemoryPropertyFlags memoryProperties, vk::BufferUsageFlags bufferUsage)
    {
        return !(memoryProperties & vk::MemoryPropertyFlagBits::eHostVisible)
                && bufferUsage & vk::BufferUsageFlagBits::eTransferDst;
    }
}

std::unique_ptr<BufferManager> BufferManager::Create(std::shared_ptr<Device> device,
        std::shared_ptr<TransferSystem> transferSystem)
{
    return std::make_unique<BufferManager>(device, transferSystem);
}

BufferManager::BufferManager(std::shared_ptr<Device> aDevice, std::shared_ptr<TransferSystem> aTransferSystem)
    : device(aDevice)
    , transferSystem(aTransferSystem)
{}

BufferManager::~BufferManager()
{
    for (auto &bufferDescriptor : buffers)
    {
        if (bufferDescriptor.type != eBufferDescriptorType::kUninitialized)
        {
            device->Get().destroyBuffer(bufferDescriptor.buffer);
            device->Get().freeMemory(bufferDescriptor.memory);
            delete[] bufferDescriptor.data;
        }
    }
}

BufferDescriptor BufferManager::CreateBuffer(const BufferProperties &properties)
{
    buffers.push_back(BufferDescriptor());
    BufferDescriptor &bufferDescriptor = buffers.back();
    bufferDescriptor.type = eBufferDescriptorType::kValid;
    bufferDescriptor.properties = properties;
    bufferDescriptor.data = new uint8_t[properties.size];

    const vk::BufferCreateInfo createInfo({}, properties.size, properties.usage,
            vk::SharingMode::eExclusive, 0, &device->GetQueueProperties().graphicsFamilyIndex);

    auto [result, buffer] = device->Get().createBuffer(createInfo);
    Assert(result == vk::Result::eSuccess);
    bufferDescriptor.buffer = buffer;

    const vk::MemoryRequirements memoryRequirements = device->Get().getBufferMemoryRequirements(buffer);
    bufferDescriptor.memory = VulkanHelpers::AllocateDeviceMemory(GetRef(device),
            memoryRequirements, properties.memoryProperties);

    result = device->Get().bindBufferMemory(buffer, bufferDescriptor.memory, 0);
    Assert(result == vk::Result::eSuccess);

    if (SBufferManager::RequireTransferSystem(properties.memoryProperties, properties.usage))
    {
        transferSystem->Reserve(properties.size);
    }

    return bufferDescriptor;
}

void BufferManager::ForceUpdate(const BufferDescriptor &)
{
    Assert(false); // TODO
}

void BufferManager::UpdateMarkedBuffers()
{
    std::vector<vk::MappedMemoryRange> memoryRanges;

    for (const auto &bufferDescriptor : buffers)
    {
        if (bufferDescriptor.type == eBufferDescriptorType::kNeedUpdate)
        {
            const vk::MemoryPropertyFlags memoryProperties = bufferDescriptor.properties.memoryProperties;
            const uint32_t size = bufferDescriptor.properties.size;

            if (memoryProperties & vk::MemoryPropertyFlagBits::eHostVisible)
            {
                VulkanHelpers::CopyToDeviceMemory(GetRef(device),
                        bufferDescriptor.AccessData<uint8_t>().data, bufferDescriptor.memory, 0, size);

                if (!(memoryProperties & vk::MemoryPropertyFlagBits::eHostCoherent))
                {
                    memoryRanges.emplace_back(bufferDescriptor.memory, 0, size);
                }
            }
            else
            {
                transferSystem->TransferBuffer(bufferDescriptor);
            }
        }
    }

    if (!memoryRanges.empty())
    {
        device->Get().flushMappedMemoryRanges(
                static_cast<uint32_t>(memoryRanges.size()), memoryRanges.data());
    }
}

BufferDescriptor BufferManager::Destroy(const BufferDescriptor &aBufferDescriptor)
{
    Assert(aBufferDescriptor.type != eBufferDescriptorType::kUninitialized);

    auto bufferDescriptor = std::find(buffers.begin(), buffers.end(), aBufferDescriptor);
    Assert(bufferDescriptor != buffers.end());

    device->Get().destroyBuffer(bufferDescriptor->buffer);
    device->Get().freeMemory(bufferDescriptor->memory);
    delete[] bufferDescriptor->data;

    bufferDescriptor->type = eBufferDescriptorType::kUninitialized;

    const BufferProperties &properties = bufferDescriptor->properties;
    if (SBufferManager::RequireTransferSystem(properties.memoryProperties, properties.usage))
    {
        transferSystem->Refuse(properties.size);
    }

    return *bufferDescriptor;
}

#include "Engine/Render/Vulkan/Resources/BufferPool.hpp"

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

#include "Utils/Helpers.hpp"

namespace SBufferPool
{
    bool RequireTransferSystem(vk::MemoryPropertyFlags memoryProperties, vk::BufferUsageFlags bufferUsage)
    {
        return !(memoryProperties & vk::MemoryPropertyFlagBits::eHostVisible)
                && bufferUsage & vk::BufferUsageFlagBits::eTransferDst;
    }
}

std::unique_ptr<BufferPool> BufferPool::Create(std::shared_ptr<Device> device,
        std::shared_ptr<TransferManager> transferManager)
{
    return std::make_unique<BufferPool>(device, transferManager);
}

BufferPool::BufferPool(std::shared_ptr<Device> aDevice, std::shared_ptr<TransferManager> aTransferManager)
    : device(aDevice)
    , transferManager(aTransferManager)
{}

BufferPool::~BufferPool()
{
    for (auto &bufferData : buffers)
    {
        if (bufferData.type != eBufferDataType::kUninitialized)
        {
            device->Get().destroyBuffer(bufferData.buffer);
            device->Get().freeMemory(bufferData.memory);
            delete[] bufferData.data;
        }
    }
}

BufferData BufferPool::CreateBuffer(const BufferProperties &properties)
{
    buffers.push_back(BufferData());
    BufferData &bufferData = buffers.back();
    bufferData.type = eBufferDataType::kValid;
    bufferData.properties = properties;
    bufferData.data = new uint8_t[properties.size];

    const vk::BufferCreateInfo createInfo({}, properties.size, properties.usage,
            vk::SharingMode::eExclusive, 0, &device->GetQueueProperties().graphicsFamilyIndex);

    auto [result, buffer] = device->Get().createBuffer(createInfo);
    Assert(result == vk::Result::eSuccess);
    bufferData.buffer = buffer;

    const vk::MemoryRequirements memoryRequirements = device->Get().getBufferMemoryRequirements(buffer);
    bufferData.memory = VulkanHelpers::AllocateDeviceMemory(GetRef(device),
            memoryRequirements, properties.memoryProperties);

    result = device->Get().bindBufferMemory(buffer, bufferData.memory, 0);
    Assert(result == vk::Result::eSuccess);

    if (SBufferPool::RequireTransferSystem(properties.memoryProperties, properties.usage))
    {
        transferManager->Reserve(properties.size);
    }

    return bufferData;
}

void BufferPool::ForceUpdate(const BufferData &)
{
    Assert(false); // TODO
}

void BufferPool::UpdateBuffers()
{
    std::vector<vk::MappedMemoryRange> memoryRanges;

    for (const auto &bufferData : buffers)
    {
        if (bufferData.type == eBufferDataType::kNeedUpdate)
        {
            const vk::MemoryPropertyFlags memoryProperties = bufferData.properties.memoryProperties;
            const uint32_t size = bufferData.properties.size;

            if (memoryProperties & vk::MemoryPropertyFlagBits::eHostVisible)
            {
                VulkanHelpers::CopyToDeviceMemory(GetRef(device),
                        bufferData.AccessData<uint8_t>().data, bufferData.memory, 0, size);

                if (!(memoryProperties & vk::MemoryPropertyFlagBits::eHostCoherent))
                {
                    memoryRanges.emplace_back(bufferData.memory, 0, size);
                }
            }
            else
            {
                transferManager->RecordBufferTransfer(bufferData);
            }
        }
    }

    if (!memoryRanges.empty())
    {
        device->Get().flushMappedMemoryRanges(
                static_cast<uint32_t>(memoryRanges.size()), memoryRanges.data());
    }
}

BufferData BufferPool::Destroy(const BufferData &aBufferData)
{
    Assert(aBufferData.type != eBufferDataType::kUninitialized);

    auto bufferData = std::find(buffers.begin(), buffers.end(), aBufferData);
    Assert(bufferData != buffers.end());

    device->Get().destroyBuffer(bufferData->buffer);
    device->Get().freeMemory(bufferData->memory);
    delete[] bufferData->data;

    bufferData->type = eBufferDataType::kUninitialized;

    const BufferProperties &properties = bufferData->properties;
    if (SBufferPool::RequireTransferSystem(properties.memoryProperties, properties.usage))
    {
        transferManager->Refuse(properties.size);
    }

    return *bufferData;
}

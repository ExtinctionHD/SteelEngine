#include "Engine/Render/Vulkan/BufferPool.hpp"

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

#include "Utils/Helpers.hpp"

std::unique_ptr<BufferPool> BufferPool::Create(std::shared_ptr<VulkanDevice> device)
{
    return std::make_unique<BufferPool>(device);
}

BufferPool::BufferPool(std::shared_ptr<VulkanDevice> aDevice)
    : device(aDevice)
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

    return bufferData;
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

    return *bufferData;
}

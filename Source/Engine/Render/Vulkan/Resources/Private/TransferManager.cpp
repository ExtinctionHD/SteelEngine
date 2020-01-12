#include "Engine/Render/Vulkan/Resources/TransferManager.hpp"

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

#include "Engine/Render/Vulkan/Resources/ImageData.hpp"
#include "Engine/Render/Vulkan/Resources/BufferData.hpp"

#include "Utils/Assert.hpp"
#include "Utils/Helpers.hpp"

namespace STransferManager
{
    std::pair<vk::Buffer, vk::DeviceMemory> CreateStagingBuffer(const Device &device, uint32_t size)
    {
        const vk::BufferCreateInfo createInfo({}, size,
                vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive,
                1, &device.GetQueueProperties().graphicsFamilyIndex);

        auto [result, buffer] = device.Get().createBuffer(createInfo);
        Assert(result == vk::Result::eSuccess);

        const vk::MemoryRequirements memoryRequirements = device.Get().getBufferMemoryRequirements(buffer);

        const vk::MemoryPropertyFlags memoryProperties
                = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
        vk::DeviceMemory memory = VulkanHelpers::AllocateDeviceMemory(device, memoryRequirements, memoryProperties);

        return { buffer, memory };
    }

    void DestroyStagingBuffer(vk::Device device, const std::pair<vk::Buffer, vk::DeviceMemory> &stagingBuffer)
    {
        const auto [buffer, memory] = stagingBuffer;

        device.destroyBuffer(buffer);
        device.freeMemory(memory);
    }
}

std::shared_ptr<TransferManager> TransferManager::Create(std::shared_ptr<Device> device,
        uint32_t capacity)
{
    return std::make_unique<TransferManager>(device, capacity);
}

TransferManager::TransferManager(std::shared_ptr<Device> aDevice, uint32_t aCapacity)
    : device(aDevice)
    , capacity(aCapacity)
{}

TransferManager::~TransferManager()
{
    STransferManager::DestroyStagingBuffer(device->Get(), stagingBuffer);
}

void TransferManager::Reserve(uint32_t aSize)
{
    size += aSize;

    if (capacity < size)
    {
        capacity = size + capacity / 2;

        STransferManager::DestroyStagingBuffer(device->Get(), stagingBuffer);
        stagingBuffer = STransferManager::CreateStagingBuffer(GetRef(device), capacity);
    }
}

void TransferManager::Refuse(uint32_t aSize)
{
    size -= aSize;
}

void TransferManager::RecordImageTransfer(const ImageData &)
{
    Assert(false); // TODO
}

void TransferManager::RecordBufferTransfer(const BufferData &bufferData)
{
    const auto [buffer, memory] = stagingBuffer;
    const uint32_t dataSize = bufferData.GetProperties().size;

    Assert(currentOffset + dataSize <= capacity);

    VulkanHelpers::CopyToDeviceMemory(GetRef(device),
            bufferData.AccessData<uint8_t>().data, memory, currentOffset, size);

    const vk::BufferCopy region(currentOffset, 0, dataSize);

    const DeviceCommands updateCommands = [&buffer, &bufferData, &region](vk::CommandBuffer commandBuffer)
        {
            commandBuffer.copyBuffer(buffer, bufferData.GetBuffer(), 1, &region);
        };

    updateCommandsList.push_back(updateCommands);
    currentOffset += dataSize;
}

void TransferManager::TransferResources()
{
    if (!updateCommandsList.empty())
    {
        const DeviceCommands commands = [this](vk::CommandBuffer commandBuffer)
            {
                for (const auto &updateCommands : updateCommandsList)
                {
                    updateCommands(commandBuffer);
                }
            };

        device->ExecuteOneTimeCommands(commands);

        updateCommandsList.clear();
    }
}

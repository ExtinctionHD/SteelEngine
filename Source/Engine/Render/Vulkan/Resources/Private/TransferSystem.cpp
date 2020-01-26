#include "Engine/Render/Vulkan/Resources/TransferSystem.hpp"

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/Image.hpp"

#include "Utils/Assert.hpp"
#include "Utils/Helpers.hpp"

namespace STransferSystem
{
    std::pair<vk::Buffer, vk::DeviceMemory> CreateStagingBuffer(const Device &device, vk::DeviceSize size)
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

        result = device.Get().bindBufferMemory(buffer, memory, 0);
        Assert(result == vk::Result::eSuccess);

        return { buffer, memory };
    }

    void DestroyStagingBuffer(vk::Device device, const std::pair<vk::Buffer, vk::DeviceMemory> &stagingBuffer)
    {
        const auto [buffer, memory] = stagingBuffer;

        device.destroyBuffer(buffer);
        device.freeMemory(memory);
    }
}

TransferSystem::TransferSystem(std::shared_ptr<Device> aDevice, vk::DeviceSize aCapacity)
    : device(aDevice)
    , capacity(aCapacity)
{
    stagingBuffer = STransferSystem::CreateStagingBuffer(GetRef(device), capacity);
}

TransferSystem::~TransferSystem()
{
    STransferSystem::DestroyStagingBuffer(device->Get(), stagingBuffer);
}

void TransferSystem::ReserveMemory(vk::DeviceSize aSize)
{
    size += aSize;

    if (capacity < size)
    {
        PerformTransfer();

        capacity = size + capacity / 2;

        STransferSystem::DestroyStagingBuffer(device->Get(), stagingBuffer);
        stagingBuffer = STransferSystem::CreateStagingBuffer(GetRef(device), capacity);
    }
}

void TransferSystem::RefuseMemory(vk::DeviceSize aSize)
{
    size -= aSize;
}

void TransferSystem::TransferImage(const Image &)
{
    Assert(false); // TODO
}

void TransferSystem::TransferBuffer(BufferHandle handle)
{
    const auto &[buffer, memory] = stagingBuffer;
    const vk::DeviceSize dataSize = handle->description.size;

    Assert(currentOffset + dataSize <= capacity);

    VulkanHelpers::CopyToDeviceMemory(GetRef(device),
            handle->AccessData<uint8_t>().data, memory, currentOffset, size);

    const vk::BufferCopy region(currentOffset, 0, dataSize);

    const DeviceCommands transferCommands = [&buffer, handle, region](vk::CommandBuffer commandBuffer)
        {
            commandBuffer.copyBuffer(buffer, handle->buffer, 1, &region);
            handle->state = eResourceState::kUpdated;
        };

    transferCommandsList.push_back(transferCommands);
    currentOffset += dataSize;
}

void TransferSystem::PerformTransfer()
{
    if (!transferCommandsList.empty())
    {
        const DeviceCommands commands = [this](vk::CommandBuffer commandBuffer)
            {
                for (const auto &transferCommands : transferCommandsList)
                {
                    transferCommands(commandBuffer);
                }
            };

        device->ExecuteOneTimeCommands(commands);

        transferCommandsList.clear();
    }
}

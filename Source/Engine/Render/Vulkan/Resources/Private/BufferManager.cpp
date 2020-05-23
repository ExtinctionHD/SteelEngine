#include "Engine/Render/Vulkan/Resources/BufferManager.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Utils/Assert.hpp"

namespace SBufferManager
{
    vk::BufferCreateInfo GetBufferCreateInfo(const BufferDescription &description)
    {
        const Queues::Description &queuesDescription = VulkanContext::device->GetQueuesDescription();

        const vk::BufferCreateInfo createInfo({}, description.size, description.usage,
                vk::SharingMode::eExclusive, 0, &queuesDescription.graphicsFamilyIndex);

        return createInfo;
    }
}

vk::Buffer BufferManager::CreateBuffer(const BufferDescription &description, BufferCreateFlags createFlags)
{
    const vk::BufferCreateInfo createInfo = SBufferManager::GetBufferCreateInfo(description);

    vk::Buffer buffer = VulkanContext::memoryManager->CreateBuffer(createInfo, description.memoryProperties);

    vk::Buffer stagingBuffer = nullptr;
    if (createFlags & BufferCreateFlagBits::eStagingBuffer)
    {
        stagingBuffer = BufferHelpers::CreateStagingBuffer(description.size);
    }

    buffers.emplace(buffer, BufferEntry{ description, stagingBuffer });

    return buffer;
}

void BufferManager::DestroyBuffer(vk::Buffer buffer)
{
    const auto &[description, stagingBuffer] = buffers.at(buffer);

    if (stagingBuffer)
    {
        VulkanContext::memoryManager->DestroyBuffer(stagingBuffer);
    }

    VulkanContext::memoryManager->DestroyBuffer(buffer);

    buffers.erase(buffers.find(buffer));
}

void BufferManager::UpdateBuffer(vk::CommandBuffer commandBuffer, vk::Buffer buffer, const ByteView &data)
{
    const auto &[description, stagingBuffer] = buffers.at(buffer);

    const vk::MemoryPropertyFlags memoryProperties = description.memoryProperties;

    if (memoryProperties & vk::MemoryPropertyFlagBits::eHostVisible)
    {
        const MemoryBlock memoryBlock = VulkanContext::memoryManager->GetBufferMemoryBlock(buffer);

        VulkanContext::memoryManager->CopyDataToMemory(data, memoryBlock);

        if (!(memoryProperties & vk::MemoryPropertyFlagBits::eHostCoherent))
        {
            const vk::MappedMemoryRange memoryRange{
                memoryBlock.memory, memoryBlock.offset, memoryBlock.size
            };

            VulkanContext::device->Get().flushMappedMemoryRanges({ memoryRange });
        }
    }
    else
    {
        Assert(commandBuffer && stagingBuffer);
        Assert(description.usage & vk::BufferUsageFlagBits::eTransferDst);

        VulkanContext::memoryManager->CopyDataToMemory(data,
                VulkanContext::memoryManager->GetBufferMemoryBlock(stagingBuffer));

        const vk::BufferCopy region(0, 0, data.size);

        commandBuffer.copyBuffer(stagingBuffer, buffer, { region });
    }
}

const BufferDescription &BufferManager::GetBufferDescription(vk::Buffer buffer) const
{
    return buffers.at(buffer).description;
}

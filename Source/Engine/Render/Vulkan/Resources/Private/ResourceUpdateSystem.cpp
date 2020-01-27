#include "Engine/Render/Vulkan/Resources/ResourceUpdateSystem.hpp"

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/Image.hpp"

#include "Utils/Assert.hpp"
#include "Utils/Helpers.hpp"

namespace SResourceUpdateSystem
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

ResourceUpdateSystem::ResourceUpdateSystem(std::shared_ptr<Device> aDevice, vk::DeviceSize aCapacity)
    : device(aDevice)
    , capacity(aCapacity)
{
    stagingBuffer = SResourceUpdateSystem::CreateStagingBuffer(GetRef(device), capacity);
}

ResourceUpdateSystem::~ResourceUpdateSystem()
{
    SResourceUpdateSystem::DestroyStagingBuffer(device->Get(), stagingBuffer);
}

DeviceCommands ResourceUpdateSystem::GetBufferUpdateCommands(BufferHandle handle)
{
    const auto &[buffer, memory] = stagingBuffer;
    const vk::DeviceSize dataSize = handle->description.size;

    Assert(currentOffset + dataSize <= capacity);

    VulkanHelpers::CopyToDeviceMemory(GetRef(device),
            handle->AccessData<uint8_t>().data, memory, currentOffset, dataSize);

    const vk::BufferCopy region(currentOffset, 0, dataSize);

    const DeviceCommands commands = [&buffer, handle, region](vk::CommandBuffer commandBuffer)
        {
            commandBuffer.copyBuffer(buffer, handle->buffer, 1, &region);
            handle->state = eResourceState::kUpdated;
        };

    currentOffset += dataSize;

    return commands;
}

void ResourceUpdateSystem::UpdateBuffer(BufferHandle handle)
{
    const DeviceCommands commands = GetBufferUpdateCommands(handle);

    updateCommandsList.push_back(commands);
}

DeviceCommands ResourceUpdateSystem::GetLayoutUpdateCommands(const ImageResource &imageResource,
        vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
    vk::AccessFlags srcAccessMask;
    switch (oldLayout)
    {
    case vk::ImageLayout::eTransferDstOptimal:
        srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        break;
    case vk::ImageLayout::ePreinitialized:
        srcAccessMask = vk::AccessFlagBits::eHostWrite;
        break;
    case vk::ImageLayout::eGeneral:
    case vk::ImageLayout::eUndefined:
        break;
    default:
        Assert(false);
    }

    vk::PipelineStageFlags srcStageMask;
    switch (oldLayout)
    {
    case vk::ImageLayout::eTransferDstOptimal:
        srcStageMask = vk::PipelineStageFlagBits::eTransfer;
        break;
    case vk::ImageLayout::eUndefined:
        srcStageMask = vk::PipelineStageFlagBits::eTopOfPipe;
        break;
    case vk::ImageLayout::eGeneral:
    case vk::ImageLayout::ePreinitialized:
        srcStageMask = vk::PipelineStageFlagBits::eHost;
        break;
    default:
        Assert(false);
    }

    vk::AccessFlags dstAccessMask;
    switch (newLayout)
    {
    case vk::ImageLayout::eColorAttachmentOptimal:
        dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        break;
    case vk::ImageLayout::eDepthStencilAttachmentOptimal:
        dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead
                | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        break;
    case vk::ImageLayout::eShaderReadOnlyOptimal:
        dstAccessMask = vk::AccessFlagBits::eShaderRead;
        break;
    case vk::ImageLayout::eTransferSrcOptimal:
        dstAccessMask = vk::AccessFlagBits::eTransferRead;
        break;
    case vk::ImageLayout::eTransferDstOptimal:
        dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        break;
    case vk::ImageLayout::eGeneral:
    case vk::ImageLayout::ePresentSrcKHR:
        break;
    default:
        Assert(false);
    }

    vk::PipelineStageFlags dstStageMask;
    switch (newLayout)
    {
    case vk::ImageLayout::eColorAttachmentOptimal:
        dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        break;
    case vk::ImageLayout::eDepthStencilAttachmentOptimal:
        dstStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests;
        break;
    case vk::ImageLayout::eGeneral:
        dstStageMask = vk::PipelineStageFlagBits::eHost;
        break;
    case vk::ImageLayout::ePresentSrcKHR:
        dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
        break;
    case vk::ImageLayout::eShaderReadOnlyOptimal:
        dstStageMask = vk::PipelineStageFlagBits::eFragmentShader;
        break;
    case vk::ImageLayout::eTransferDstOptimal:
    case vk::ImageLayout::eTransferSrcOptimal:
        dstStageMask = vk::PipelineStageFlagBits::eTransfer;
        break;
    default:
        assert(false);
        break;
    }

    const DeviceCommands commands = [=](vk::CommandBuffer commandBuffer)
        {
            const vk::ImageMemoryBarrier imageMemoryBarrier(srcAccessMask, dstAccessMask,
                    oldLayout, newLayout, 0, 0, imageResource.image, imageResource.range);

            commandBuffer.pipelineBarrier(srcStageMask, dstStageMask, {},
                    0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
        };

    return commands;
}

void ResourceUpdateSystem::UpdateLayout(const ImageResource &imageResource,
        vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
    const DeviceCommands commands = GetLayoutUpdateCommands(imageResource, oldLayout, newLayout);

    updateCommandsList.push_back(commands);
}

void ResourceUpdateSystem::ExecuteUpdateCommands()
{
    if (!updateCommandsList.empty())
    {
        const DeviceCommands deviceCommands = [this](vk::CommandBuffer commandBuffer)
            {
                for (const auto &updateCommands : updateCommandsList)
                {
                    updateCommands(commandBuffer);
                }
            };

        device->ExecuteOneTimeCommands(deviceCommands);

        updateCommandsList.clear();
    }
}

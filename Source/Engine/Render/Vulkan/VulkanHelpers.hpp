#pragma once

class Device;
class Swapchain;
class RenderPass;

namespace VulkanHelpers
{
    struct Dependency
    {
        vk::PipelineStageFlags srcStageMask;
        vk::PipelineStageFlags dstStageMask;
        vk::AccessFlags srcAccessMask;
        vk::AccessFlags dstAccessMask;
    };

    const vk::ComponentMapping kComponentMappingRgba(
            vk::ComponentSwizzle::eR,
            vk::ComponentSwizzle::eG,
            vk::ComponentSwizzle::eB,
            vk::ComponentSwizzle::eA);

    const vk::ImageSubresourceRange kSubresourceRangeColor(
            vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

    const vk::ColorComponentFlags kColorComponentFlagsRgba
            = vk::ColorComponentFlagBits::eR
            | vk::ColorComponentFlagBits::eG
            | vk::ColorComponentFlagBits::eB
            | vk::ColorComponentFlagBits::eA;

    bool IsDepthFormat(vk::Format format);

    vk::DeviceMemory AllocateDeviceMemory(const Device &device,
            vk::MemoryRequirements requirements, vk::MemoryPropertyFlags properties);

    void CopyToDeviceMemory(const Device &device, const uint8_t *src,
            vk::DeviceMemory memory, vk::DeviceSize memoryOffset, vk::DeviceSize size);

    vk::Semaphore CreateSemaphore(const Device &device);

    vk::Fence CreateFence(const Device &device, vk::FenceCreateFlags flags);

    uint32_t GetFormatTexelSize(vk::Format format);

    vk::ImageSubresourceLayers GetSubresourceLayers(const vk::ImageSubresource &subresource);

    vk::ImageSubresourceRange GetSubresourceRange(const vk::ImageSubresource &subresource);

    std::vector<vk::Framebuffer> CreateSwapchainFramebuffers(const Device &device,
            const Swapchain &swapchain, const RenderPass &renderPass);
}

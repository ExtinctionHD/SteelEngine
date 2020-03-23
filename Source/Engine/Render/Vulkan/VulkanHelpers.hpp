#pragma once

struct SyncScope
{
    vk::PipelineStageFlags stages;
    vk::AccessFlags access;
};

struct PipelineBarrier
{
    SyncScope waitedScope;
    SyncScope blockedScope;
};

struct ImageLayoutTransition
{
    vk::ImageLayout oldLayout;
    vk::ImageLayout newLayout;
    PipelineBarrier pipelineBarrier;
};

using VertexFormat = std::vector<vk::Format>;

using DeviceCommands = std::function<void(vk::CommandBuffer)>;

enum class CommandBufferType
{
    eOneTime,
    eLongLived
};

struct CommandBufferSync
{
    std::vector<vk::Semaphore> waitSemaphores;
    std::vector<vk::PipelineStageFlags> waitStages;
    std::vector<vk::Semaphore> signalSemaphores;
    vk::Fence fence;
};

namespace VulkanHelpers
{
    const vk::ComponentMapping kComponentMappingRgba(
            vk::ComponentSwizzle::eR,
            vk::ComponentSwizzle::eG,
            vk::ComponentSwizzle::eB,
            vk::ComponentSwizzle::eA);

    const vk::ImageSubresourceRange kSubresourceRangeFlatColor(
            vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

    const vk::ColorComponentFlags kColorComponentFlagsRgba
            = vk::ColorComponentFlagBits::eR
            | vk::ColorComponentFlagBits::eG
            | vk::ColorComponentFlagBits::eB
            | vk::ColorComponentFlagBits::eA;

    bool IsDepthFormat(vk::Format format);

    uint32_t GetFormatTexelSize(vk::Format format);

    vk::Semaphore CreateSemaphore(vk::Device device);

    vk::Fence CreateFence(vk::Device device, vk::FenceCreateFlags flags);

    void DestroyCommandBufferSync(vk::Device device, const CommandBufferSync &sync);

    vk::ImageSubresourceLayers GetSubresourceLayers(const vk::ImageSubresource &subresource);

    vk::ImageSubresourceRange GetSubresourceRange(const vk::ImageSubresource &subresource);

    std::vector<vk::Framebuffer> CreateSwapchainFramebuffers(vk::Device device, vk::RenderPass renderPass,
            const std::vector<vk::ImageView> &imageViews, const vk::Extent2D &extent);

    vk::PipelineLayout CreatePipelineLayout(vk::Device device,
            const std::vector<vk::DescriptorSetLayout> &layouts,
            const std::vector<vk::PushConstantRange> &pushConstantRanges);

    void TransitImageLayout(vk::CommandBuffer commandBuffer, vk::Image image,
            const vk::ImageSubresourceRange &subresourceRange, const ImageLayoutTransition &transition);

    void SubmitCommandBuffer(vk::Queue queue, vk::CommandBuffer commandBuffer,
            DeviceCommands deviceCommands, const CommandBufferSync &sync);

    void WaitForFences(vk::Device device, std::vector<vk::Fence> fences);
}

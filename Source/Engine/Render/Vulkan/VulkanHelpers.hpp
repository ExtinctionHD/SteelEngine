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
    vk::Extent3D GetExtent3D(const vk::Extent2D &extent2D);

    vk::Semaphore CreateSemaphore(vk::Device device);

    vk::Fence CreateFence(vk::Device device, vk::FenceCreateFlags flags);

    void DestroyCommandBufferSync(vk::Device device, const CommandBufferSync &sync);

    std::vector<vk::Framebuffer> CreateSwapchainFramebuffers(vk::Device device,
            vk::RenderPass renderPass, const vk::Extent2D &extent,
            const std::vector<vk::ImageView> &swapchainImageViews,
            const std::vector<vk::ImageView> &otherImageViews);

    vk::PipelineLayout CreatePipelineLayout(vk::Device device,
            const std::vector<vk::DescriptorSetLayout> &layouts,
            const std::vector<vk::PushConstantRange> &pushConstantRanges);

    void SubmitCommandBuffer(vk::Queue queue, vk::CommandBuffer commandBuffer,
            DeviceCommands deviceCommands, const CommandBufferSync &sync);

    void WaitForFences(vk::Device device, std::vector<vk::Fence> fences);
}

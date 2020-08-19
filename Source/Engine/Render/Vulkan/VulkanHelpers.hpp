#pragma once

#include "Utils/Assert.hpp"

struct SyncScope
{
    static const SyncScope kWaitForNothing;
    static const SyncScope kNothingToBlock;
    static const SyncScope kTransferWrite;
    static const SyncScope kTransferRead;
    static const SyncScope kVerticesRead;
    static const SyncScope kIndicesRead;
    static const SyncScope kAccelerationStructureBuild;
    static const SyncScope kRayTracingShaderWrite;
    static const SyncScope kRayTracingShaderRead;
    static const SyncScope kVertexShaderRead;
    static const SyncScope kFragmentShaderRead;
    static const SyncScope kShaderRead;
    static const SyncScope kColorAttachmentWrite;
    static const SyncScope KDepthStencilAttachmentWrite;
    static const SyncScope KComputeShaderWrite;
    static const SyncScope KComputeShaderRead;

    vk::PipelineStageFlags stages;
    vk::AccessFlags access;

    SyncScope operator|(const SyncScope& other) const;
};

struct PipelineBarrier
{
    SyncScope waitedScope;
    SyncScope blockedScope;
};

using VertexFormat = std::vector<vk::Format>;

using DeviceCommands = std::function<void(vk::CommandBuffer)>;
using RenderCommands = std::function<void(vk::CommandBuffer, uint32_t)>;

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
    const vk::PipelineStageFlags kShaderPipelineStages = vk::PipelineStageFlagBits::eVertexShader
            | vk::PipelineStageFlagBits::eFragmentShader | vk::PipelineStageFlagBits::eRayTracingShaderKHR;

    vk::Extent3D GetExtent3D(const vk::Extent2D& extent2D);

    vk::Semaphore CreateSemaphore(vk::Device device);

    vk::Fence CreateFence(vk::Device device, vk::FenceCreateFlags flags);

    void DestroyCommandBufferSync(vk::Device device, const CommandBufferSync& sync);

    std::vector<vk::Framebuffer> CreateSwapchainFramebuffers(vk::Device device,
            vk::RenderPass renderPass, const vk::Extent2D& extent,
            const std::vector<vk::ImageView>& swapchainImageViews,
            const std::vector<vk::ImageView>& additionalImageViews);

    vk::PipelineLayout CreatePipelineLayout(vk::Device device,
            const std::vector<vk::DescriptorSetLayout>& layouts,
            const std::vector<vk::PushConstantRange>& pushConstantRanges);

    void SubmitCommandBuffer(vk::Queue queue, vk::CommandBuffer commandBuffer,
            DeviceCommands deviceCommands, const CommandBufferSync& sync);

    void WaitForFences(vk::Device device, std::vector<vk::Fence> fences);

    template <class T>
    vk::Extent2D GetExtent(T width, T height)
    {
        static_assert(std::is_arithmetic<T>());
        return vk::Extent2D(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    }

    template <class T>
    void SetObjectName(vk::Device device, T object, const std::string& name)
    {
        const uint64_t objectHandle = reinterpret_cast<uint64_t>(static_cast<typename T::CType>(object));

        const vk::DebugUtilsObjectNameInfoEXT nameInfo(T::objectType, objectHandle, name.c_str());

        const vk::Result result = device.setDebugUtilsObjectNameEXT(nameInfo);
        Assert(result == vk::Result::eSuccess);
    }
}

#pragma once

#include "Engine/Config.hpp"

#include "Utils/Assert.hpp"

struct SyncScope
{
    static const SyncScope kWaitForNone;
    static const SyncScope kWaitForAll;
    static const SyncScope kBlockNone;
    static const SyncScope kBlockAll;
    static const SyncScope kTransferWrite;
    static const SyncScope kTransferRead;
    static const SyncScope kVerticesRead;
    static const SyncScope kIndicesRead;
    static const SyncScope kAccelerationStructureWrite;
    static const SyncScope kAccelerationStructureRead;
    static const SyncScope kRayTracingShaderWrite;
    static const SyncScope kRayTracingShaderRead;
    static const SyncScope kRayTracingUniformRead;
    static const SyncScope kComputeShaderWrite;
    static const SyncScope kComputeShaderRead;
    static const SyncScope kComputeUniformRead;
    static const SyncScope kVertexShaderRead;
    static const SyncScope kVertexUniformRead;
    static const SyncScope kFragmentShaderRead;
    static const SyncScope kFragmentUniformRead;
    static const SyncScope kShaderRead;
    static const SyncScope kUniformRead;
    static const SyncScope kColorAttachmentWrite;
    static const SyncScope kDepthStencilAttachmentWrite;
    static const SyncScope kDepthStencilAttachmentRead;

    vk::PipelineStageFlags stages;
    vk::AccessFlags access;

    SyncScope operator|(const SyncScope& other) const;
};

struct PipelineBarrier
{
    static const PipelineBarrier kEmpty;
    static const PipelineBarrier kFull;

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
    constexpr vk::PipelineStageFlags kShaderPipelineStages = vk::PipelineStageFlagBits::eComputeShader | vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eFragmentShader | vk::PipelineStageFlagBits::eRayTracingShaderKHR;

    constexpr vk::ClearDepthStencilValue kDefaultClearDepthStencilValue(Config::engine.kReverseDepth ? 0.0f : 1.0f, 0);

    const vk::ClearColorValue kDefaultClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 0.0f });

    vk::Extent3D GetExtent3D(const vk::Extent2D& extent2D);

    vk::Extent2D GetExtent2D(const vk::Extent3D& extent3D);

    vk::Semaphore CreateSemaphore(vk::Device device);

    vk::Fence CreateFence(vk::Device device, vk::FenceCreateFlags flags);

    void DestroyCommandBufferSync(vk::Device device, const CommandBufferSync& sync);

    std::vector<vk::Framebuffer> CreateFramebuffers(
            vk::Device device, vk::RenderPass renderPass, const vk::Extent2D& extent,
            const std::vector<std::vector<vk::ImageView>>& separateImageViews,
            const std::vector<vk::ImageView>& commonImageViews);

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

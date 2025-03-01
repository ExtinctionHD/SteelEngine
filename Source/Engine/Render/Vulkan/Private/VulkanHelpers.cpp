#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

#include "Engine/ConsoleVariable.hpp"

const SyncScope SyncScope::kWaitForNone{
    vk::PipelineStageFlagBits::eTopOfPipe,
    vk::AccessFlagBits::eNone,
};

const SyncScope SyncScope::kWaitForAll{
    vk::PipelineStageFlagBits::eAllCommands,
    vk::AccessFlagBits::eMemoryWrite,
};

const SyncScope SyncScope::kBlockNone{
    vk::PipelineStageFlagBits::eBottomOfPipe,
    vk::AccessFlagBits::eNone,
};

const SyncScope SyncScope::kBlockAll{
    vk::PipelineStageFlagBits::eAllCommands,
    vk::AccessFlagBits::eMemoryWrite | vk::AccessFlagBits::eMemoryRead,
};

const SyncScope SyncScope::kTransferWrite{
    vk::PipelineStageFlagBits::eTransfer,
    vk::AccessFlagBits::eTransferWrite
};

const SyncScope SyncScope::kTransferRead{
    vk::PipelineStageFlagBits::eTransfer,
    vk::AccessFlagBits::eTransferRead
};

const SyncScope SyncScope::kVerticesRead{
    vk::PipelineStageFlagBits::eVertexInput,
    vk::AccessFlagBits::eVertexAttributeRead
};

const SyncScope SyncScope::kIndicesRead{
    vk::PipelineStageFlagBits::eVertexInput,
    vk::AccessFlagBits::eIndexRead
};

const SyncScope SyncScope::kAccelerationStructureWrite{
    vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
    vk::AccessFlagBits::eAccelerationStructureWriteKHR
};

const SyncScope SyncScope::kAccelerationStructureRead{
    vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
    vk::AccessFlagBits::eAccelerationStructureReadKHR
};

const SyncScope SyncScope::kAccelerationStructureShaderRead{
    vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
    vk::AccessFlagBits::eShaderRead
};

const SyncScope SyncScope::kRayTracingAccelerationStructureRead{
    vk::PipelineStageFlagBits::eRayTracingShaderKHR,
    vk::AccessFlagBits::eAccelerationStructureReadKHR
};

const SyncScope SyncScope::kRayTracingShaderWrite{
    vk::PipelineStageFlagBits::eRayTracingShaderKHR,
    vk::AccessFlagBits::eShaderWrite
};

const SyncScope SyncScope::kRayTracingShaderRead{
    vk::PipelineStageFlagBits::eRayTracingShaderKHR,
    vk::AccessFlagBits::eShaderRead
};

const SyncScope SyncScope::kRayTracingUniformRead{
    vk::PipelineStageFlagBits::eRayTracingShaderKHR,
    vk::AccessFlagBits::eUniformRead
};

const SyncScope SyncScope::kComputeShaderWrite{
    vk::PipelineStageFlagBits::eComputeShader,
    vk::AccessFlagBits::eShaderWrite
};

const SyncScope SyncScope::kComputeShaderRead{
    vk::PipelineStageFlagBits::eComputeShader,
    vk::AccessFlagBits::eShaderRead
};

const SyncScope SyncScope::kComputeUniformRead{
    vk::PipelineStageFlagBits::eComputeShader,
    vk::AccessFlagBits::eUniformRead
};

const SyncScope SyncScope::kVertexShaderRead{
    vk::PipelineStageFlagBits::eVertexShader,
    vk::AccessFlagBits::eShaderRead
};

const SyncScope SyncScope::kVertexUniformRead{
    vk::PipelineStageFlagBits::eVertexShader,
    vk::AccessFlagBits::eUniformRead
};

const SyncScope SyncScope::kFragmentShaderRead{
    vk::PipelineStageFlagBits::eFragmentShader,
    vk::AccessFlagBits::eShaderRead
};

const SyncScope SyncScope::kFragmentUniformRead{
    vk::PipelineStageFlagBits::eFragmentShader,
    vk::AccessFlagBits::eUniformRead
};

const SyncScope SyncScope::kShaderRead{
    VulkanHelpers::kShaderPipelineStages,
    vk::AccessFlagBits::eShaderRead
};

const SyncScope SyncScope::kUniformRead{
    VulkanHelpers::kShaderPipelineStages,
    vk::AccessFlagBits::eUniformRead
};

const SyncScope SyncScope::kColorAttachmentWrite{
    vk::PipelineStageFlagBits::eColorAttachmentOutput,
    vk::AccessFlagBits::eColorAttachmentWrite
};

const SyncScope SyncScope::kColorAttachmentRead{
    vk::PipelineStageFlagBits::eColorAttachmentOutput,
    vk::AccessFlagBits::eColorAttachmentRead
};

const SyncScope SyncScope::kDepthStencilAttachmentWrite{
    vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
    vk::AccessFlagBits::eDepthStencilAttachmentWrite
};
const SyncScope SyncScope::kDepthStencilAttachmentRead{
    vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
    vk::AccessFlagBits::eDepthStencilAttachmentRead
};

const PipelineBarrier PipelineBarrier::kEmpty{
    SyncScope::kWaitForNone,
    SyncScope::kBlockNone
};

const PipelineBarrier PipelineBarrier::kFull{
    SyncScope::kWaitForAll,
    SyncScope::kBlockAll
};

SyncScope SyncScope::operator|(const SyncScope& other) const
{
    return SyncScope{ stages | other.stages, access | other.access };
}

vk::ClearDepthStencilValue VulkanHelpers::GetDefaultClearDepthStencilValue()
{
    static const CVarBool& reversedDepthCVar = CVarBool::Get("r.ReversedDepth");

    const bool reversedDepth = reversedDepthCVar.GetValue();

    return vk::ClearDepthStencilValue(reversedDepth ? 0.0f : 1.0f, 0);
}

vk::Extent2D VulkanHelpers::GetExtent2D(const vk::Extent3D& extent3D)
{
    return vk::Extent2D(extent3D.width, extent3D.height);
}

vk::Extent3D VulkanHelpers::GetExtent3D(const vk::Extent2D& extent2D)
{
    return vk::Extent3D(extent2D.width, extent2D.height, 1);
}

vk::Extent3D VulkanHelpers::GetExtent3D(const vk::Extent2D& extent2D, uint32_t depth)
{
    return vk::Extent3D(extent2D.width, extent2D.height, depth);
}

vk::Semaphore VulkanHelpers::CreateSemaphore(vk::Device device)
{
    const vk::SemaphoreCreateInfo createInfo{};

    const auto [result, semaphore] = device.createSemaphore(createInfo);
    Assert(result == vk::Result::eSuccess);

    return semaphore;
}

vk::Fence VulkanHelpers::CreateFence(vk::Device device, vk::FenceCreateFlags flags)
{
    const vk::FenceCreateInfo createInfo(flags);

    const auto [result, fence] = device.createFence(createInfo);
    Assert(result == vk::Result::eSuccess);

    return fence;
}

void VulkanHelpers::DestroyCommandBufferSync(vk::Device device, const CommandBufferSync& sync)
{
    for (const auto& semaphore : sync.waitSemaphores)
    {
        device.destroySemaphore(semaphore);
    }
    for (const auto& semaphore : sync.signalSemaphores)
    {
        device.destroySemaphore(semaphore);
    }
    device.destroyFence(sync.fence);
}

std::vector<vk::Framebuffer> VulkanHelpers::CreateFramebuffers(
        vk::Device device, vk::RenderPass renderPass, const vk::Extent2D& extent,
        const std::vector<std::vector<vk::ImageView>>& sliceImageViews,
        const std::vector<vk::ImageView>& commonImageViews)
{
    for (const auto& imageViews : sliceImageViews)
    {
        Assert(imageViews.size() == sliceImageViews.front().size());
    }

    const size_t framebufferCount = sliceImageViews.empty() ? 1 : sliceImageViews.front().size();

    std::vector<vk::Framebuffer> framebuffers;
    framebuffers.reserve(framebufferCount);

    for (size_t i = 0; i < framebufferCount; ++i)
    {
        std::vector<vk::ImageView> framebufferImageViews;

        for (const auto& imageViews : sliceImageViews)
        {
            framebufferImageViews.push_back(imageViews[i]);
        }

        framebufferImageViews.insert(framebufferImageViews.end(), commonImageViews.begin(), commonImageViews.end());

        const vk::FramebufferCreateInfo createInfo({}, renderPass,
                static_cast<uint32_t>(framebufferImageViews.size()),
                framebufferImageViews.data(),
                extent.width, extent.height, 1);

        const auto [result, framebuffer] = device.createFramebuffer(createInfo);
        Assert(result == vk::Result::eSuccess);

        framebuffers.push_back(framebuffer);
    }

    return framebuffers;
}

vk::PipelineLayout VulkanHelpers::CreatePipelineLayout(vk::Device device,
        const std::vector<vk::DescriptorSetLayout>& layouts,
        const std::vector<vk::PushConstantRange>& pushConstantRanges)
{
    const vk::PipelineLayoutCreateInfo createInfo({},
            static_cast<uint32_t>(layouts.size()), layouts.data(),
            static_cast<uint32_t>(pushConstantRanges.size()), pushConstantRanges.data());

    const auto [result, layout] = device.createPipelineLayout(createInfo);
    Assert(result == vk::Result::eSuccess);

    return layout;
}

void VulkanHelpers::SubmitCommandBuffer(vk::Queue queue, vk::CommandBuffer commandBuffer,
        DeviceCommands deviceCommands, const CommandBufferSync& sync)
{
    const auto& [waitSemaphores, waitStages, signalSemaphores, fence] = sync;
    Assert(waitSemaphores.size() == waitStages.size());

    const vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

    vk::Result result = commandBuffer.begin(beginInfo);
    Assert(result == vk::Result::eSuccess);

    deviceCommands(commandBuffer);

    result = commandBuffer.end();
    Assert(result == vk::Result::eSuccess);

    const std::vector<vk::CommandBuffer> commandBuffers{ commandBuffer };

    const vk::SubmitInfo submitInfo(waitSemaphores, waitStages, commandBuffers, signalSemaphores);

    result = queue.submit({ submitInfo }, fence);
    Assert(result == vk::Result::eSuccess);
}

void VulkanHelpers::WaitForFences(vk::Device device, std::vector<vk::Fence> fences)
{
    constexpr uint64_t timeout = std::numeric_limits<uint64_t>::max();

    while (device.waitForFences(fences, true, timeout) == vk::Result::eTimeout) {}
}

void VulkanHelpers::InsertMemoryBarrier(vk::CommandBuffer commandBuffer, const PipelineBarrier& barrier)
{
    const vk::MemoryBarrier memoryBarrier{ barrier.waitedScope.access, barrier.blockedScope.access };

    commandBuffer.pipelineBarrier(barrier.waitedScope.stages, barrier.blockedScope.stages,
            vk::DependencyFlags(), { memoryBarrier }, {}, {});
}

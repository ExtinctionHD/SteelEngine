#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

#include "Utils/Assert.hpp"
#include "Utils/Helpers.hpp"

const SyncScope SyncScope::kWaitForNothing{
    vk::PipelineStageFlagBits::eTopOfPipe,
    vk::AccessFlags(),
};

const SyncScope SyncScope::kNothingToBlock{
    vk::PipelineStageFlagBits::eBottomOfPipe,
    vk::AccessFlags(),
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

const SyncScope SyncScope::kAccelerationStructureBuild{
    vk::PipelineStageFlagBits::eAccelerationStructureBuildNV,
    vk::AccessFlagBits::eAccelerationStructureReadNV
};

const SyncScope SyncScope::kRayTracingShaderWrite{
    vk::PipelineStageFlagBits::eRayTracingShaderNV,
    vk::AccessFlagBits::eShaderWrite
};

const SyncScope SyncScope::kRayTracingShaderRead{
    vk::PipelineStageFlagBits::eRayTracingShaderNV,
    vk::AccessFlagBits::eShaderRead
};

const SyncScope SyncScope::kVertexShaderRead{
    vk::PipelineStageFlagBits::eVertexShader,
    vk::AccessFlagBits::eShaderRead
};

const SyncScope SyncScope::kFragmentShaderRead{
    vk::PipelineStageFlagBits::eFragmentShader,
    vk::AccessFlagBits::eShaderRead
};

const SyncScope SyncScope::kShaderRead{
    VulkanHelpers::kShaderPipelineStages,
    vk::AccessFlagBits::eShaderRead
};

const SyncScope SyncScope::kColorAttachmentWrite{
    vk::PipelineStageFlagBits::eColorAttachmentOutput,
    vk::AccessFlagBits::eColorAttachmentWrite
};

const SyncScope SyncScope::KDepthStencilAttachmentWrite{
    vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
    vk::AccessFlagBits::eDepthStencilAttachmentWrite
};

const SyncScope SyncScope::KComputeShaderWrite{
    vk::PipelineStageFlagBits::eComputeShader,
    vk::AccessFlagBits::eShaderWrite
};

const SyncScope SyncScope::KComputeShaderRead{
    vk::PipelineStageFlagBits::eComputeShader,
    vk::AccessFlagBits::eShaderRead
};

SyncScope SyncScope::operator|(const SyncScope &other) const
{
    return SyncScope{ stages | other.stages, access | other.access };
}

vk::Extent3D VulkanHelpers::GetExtent3D(const vk::Extent2D &extent2D)
{
    return vk::Extent3D(extent2D.width, extent2D.height, 1);
}

vk::Semaphore VulkanHelpers::CreateSemaphore(vk::Device device)
{
    const vk::SemaphoreCreateInfo createInfo({});

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

void VulkanHelpers::DestroyCommandBufferSync(vk::Device device, const CommandBufferSync &sync)
{
    for (const auto &semaphore : sync.waitSemaphores)
    {
        device.destroySemaphore(semaphore);
    }
    for (const auto &semaphore : sync.signalSemaphores)
    {
        device.destroySemaphore(semaphore);
    }
    device.destroyFence(sync.fence);
}

std::vector<vk::Framebuffer> VulkanHelpers::CreateSwapchainFramebuffers(vk::Device device,
        vk::RenderPass renderPass, const vk::Extent2D &extent,
        const std::vector<vk::ImageView> &swapchainImageViews,
        const std::vector<vk::ImageView> &additionalImageViews)
{
    std::vector<vk::Framebuffer> framebuffers;
    framebuffers.reserve(swapchainImageViews.size());

    for (const auto &swapchainImageView : swapchainImageViews)
    {
        std::vector<vk::ImageView> imageViews{ swapchainImageView };
        imageViews.insert(imageViews.end(), additionalImageViews.begin(), additionalImageViews.end());

        const vk::FramebufferCreateInfo createInfo({}, renderPass,
                static_cast<uint32_t>(imageViews.size()), imageViews.data(),
                extent.width, extent.height, 1);

        const auto [result, framebuffer] = device.createFramebuffer(createInfo);
        Assert(result == vk::Result::eSuccess);

        framebuffers.push_back(framebuffer);
    }

    return framebuffers;
}

vk::PipelineLayout VulkanHelpers::CreatePipelineLayout(vk::Device device,
        const std::vector<vk::DescriptorSetLayout> &layouts,
        const std::vector<vk::PushConstantRange> &pushConstantRanges)
{
    const vk::PipelineLayoutCreateInfo createInfo({},
            static_cast<uint32_t>(layouts.size()), layouts.data(),
            static_cast<uint32_t>(pushConstantRanges.size()), pushConstantRanges.data());

    const auto [result, layout] = device.createPipelineLayout(createInfo);
    Assert(result == vk::Result::eSuccess);

    return layout;
}

void VulkanHelpers::SubmitCommandBuffer(vk::Queue queue, vk::CommandBuffer commandBuffer,
        DeviceCommands deviceCommands, const CommandBufferSync &sync)
{
    const auto &[waitSemaphores, waitStages, signalSemaphores, fence] = sync;

    const vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

    vk::Result result = commandBuffer.begin(beginInfo);
    Assert(result == vk::Result::eSuccess);

    deviceCommands(commandBuffer);

    result = commandBuffer.end();
    Assert(result == vk::Result::eSuccess);

    const vk::SubmitInfo submitInfo(static_cast<uint32_t>(waitSemaphores.size()),
            waitSemaphores.data(), waitStages.data(), 1, &commandBuffer,
            static_cast<uint32_t>(signalSemaphores.size()), signalSemaphores.data());

    result = queue.submit(1, &submitInfo, fence);
    Assert(result == vk::Result::eSuccess);
}

void VulkanHelpers::WaitForFences(vk::Device device, std::vector<vk::Fence> fences)
{
    while (device.waitForFences(fences, true, Numbers::kMaxUint) == vk::Result::eTimeout) {}
}

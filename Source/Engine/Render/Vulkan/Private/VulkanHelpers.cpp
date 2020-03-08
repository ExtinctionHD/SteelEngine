#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Swapchain.hpp"
#include "Engine/Render/Vulkan/RenderPass.hpp"

#include "Utils/Assert.hpp"

bool VulkanHelpers::IsDepthFormat(vk::Format format)
{
    switch (format)
    {
    case vk::Format::eD16Unorm:
    case vk::Format::eX8D24UnormPack32:
    case vk::Format::eD32Sfloat:
    case vk::Format::eD16UnormS8Uint:
    case vk::Format::eD24UnormS8Uint:
    case vk::Format::eD32SfloatS8Uint:
        return true;
    default:
        return false;
    }
}

vk::Semaphore VulkanHelpers::CreateSemaphore(const Device &device)
{
    const auto [result, semaphore] = device.Get().createSemaphore({});
    Assert(result == vk::Result::eSuccess);

    return semaphore;
}

vk::Fence VulkanHelpers::CreateFence(const Device &device, vk::FenceCreateFlags flags)
{
    const auto [result, fence] = device.Get().createFence({ flags });
    Assert(result == vk::Result::eSuccess);

    return fence;
}

uint32_t VulkanHelpers::GetFormatTexelSize(vk::Format format)
{
    switch (format)
    {
    case vk::Format::eUndefined:
        return 0;

    case vk::Format::eR4G4UnormPack8:
        return 1;

    case vk::Format::eR4G4B4A4UnormPack16:
    case vk::Format::eB4G4R4A4UnormPack16:
    case vk::Format::eR5G6B5UnormPack16:
    case vk::Format::eB5G6R5UnormPack16:
    case vk::Format::eR5G5B5A1UnormPack16:
    case vk::Format::eB5G5R5A1UnormPack16:
    case vk::Format::eA1R5G5B5UnormPack16:
        return 2;

    case vk::Format::eR8Unorm:
    case vk::Format::eR8Snorm:
    case vk::Format::eR8Uscaled:
    case vk::Format::eR8Sscaled:
    case vk::Format::eR8Uint:
    case vk::Format::eR8Sint:
    case vk::Format::eR8Srgb:
        return 1;

    case vk::Format::eR8G8Unorm:
    case vk::Format::eR8G8Snorm:
    case vk::Format::eR8G8Uscaled:
    case vk::Format::eR8G8Sscaled:
    case vk::Format::eR8G8Uint:
    case vk::Format::eR8G8Sint:
    case vk::Format::eR8G8Srgb:
        return 2;

    case vk::Format::eR8G8B8Unorm:
    case vk::Format::eR8G8B8Snorm:
    case vk::Format::eR8G8B8Uscaled:
    case vk::Format::eR8G8B8Sscaled:
    case vk::Format::eR8G8B8Uint:
    case vk::Format::eR8G8B8Sint:
    case vk::Format::eR8G8B8Srgb:
    case vk::Format::eB8G8R8Unorm:
    case vk::Format::eB8G8R8Snorm:
    case vk::Format::eB8G8R8Uscaled:
    case vk::Format::eB8G8R8Sscaled:
    case vk::Format::eB8G8R8Uint:
    case vk::Format::eB8G8R8Sint:
    case vk::Format::eB8G8R8Srgb:
        return 3;

    case vk::Format::eR8G8B8A8Unorm:
    case vk::Format::eR8G8B8A8Snorm:
    case vk::Format::eR8G8B8A8Uscaled:
    case vk::Format::eR8G8B8A8Sscaled:
    case vk::Format::eR8G8B8A8Uint:
    case vk::Format::eR8G8B8A8Sint:
    case vk::Format::eR8G8B8A8Srgb:
    case vk::Format::eB8G8R8A8Unorm:
    case vk::Format::eB8G8R8A8Snorm:
    case vk::Format::eB8G8R8A8Uscaled:
    case vk::Format::eB8G8R8A8Sscaled:
    case vk::Format::eB8G8R8A8Uint:
    case vk::Format::eB8G8R8A8Sint:
    case vk::Format::eB8G8R8A8Srgb:
    case vk::Format::eA8B8G8R8UnormPack32:
    case vk::Format::eA8B8G8R8SnormPack32:
    case vk::Format::eA8B8G8R8UscaledPack32:
    case vk::Format::eA8B8G8R8SscaledPack32:
    case vk::Format::eA8B8G8R8UintPack32:
    case vk::Format::eA8B8G8R8SintPack32:
    case vk::Format::eA8B8G8R8SrgbPack32:
        return 4;

    case vk::Format::eA2R10G10B10UnormPack32:
    case vk::Format::eA2R10G10B10SnormPack32:
    case vk::Format::eA2R10G10B10UscaledPack32:
    case vk::Format::eA2R10G10B10SscaledPack32:
    case vk::Format::eA2R10G10B10UintPack32:
    case vk::Format::eA2R10G10B10SintPack32:
    case vk::Format::eA2B10G10R10UnormPack32:
    case vk::Format::eA2B10G10R10SnormPack32:
    case vk::Format::eA2B10G10R10UscaledPack32:
    case vk::Format::eA2B10G10R10SscaledPack32:
    case vk::Format::eA2B10G10R10UintPack32:
    case vk::Format::eA2B10G10R10SintPack32:
        return 4;

    case vk::Format::eR16Unorm:
    case vk::Format::eR16Snorm:
    case vk::Format::eR16Uscaled:
    case vk::Format::eR16Sscaled:
    case vk::Format::eR16Uint:
    case vk::Format::eR16Sint:
    case vk::Format::eR16Sfloat:
        return 2;

    case vk::Format::eR16G16Unorm:
    case vk::Format::eR16G16Snorm:
    case vk::Format::eR16G16Uscaled:
    case vk::Format::eR16G16Sscaled:
    case vk::Format::eR16G16Uint:
    case vk::Format::eR16G16Sint:
    case vk::Format::eR16G16Sfloat:
        return 4;

    case vk::Format::eR16G16B16Unorm:
    case vk::Format::eR16G16B16Snorm:
    case vk::Format::eR16G16B16Uscaled:
    case vk::Format::eR16G16B16Sscaled:
    case vk::Format::eR16G16B16Uint:
    case vk::Format::eR16G16B16Sint:
    case vk::Format::eR16G16B16Sfloat:
        return 6;

    case vk::Format::eR16G16B16A16Unorm:
    case vk::Format::eR16G16B16A16Snorm:
    case vk::Format::eR16G16B16A16Uscaled:
    case vk::Format::eR16G16B16A16Sscaled:
    case vk::Format::eR16G16B16A16Uint:
    case vk::Format::eR16G16B16A16Sint:
    case vk::Format::eR16G16B16A16Sfloat:
        return 8;

    case vk::Format::eR32Uint:
    case vk::Format::eR32Sint:
    case vk::Format::eR32Sfloat:
        return 4;

    case vk::Format::eR32G32Uint:
    case vk::Format::eR32G32Sint:
    case vk::Format::eR32G32Sfloat:
        return 8;

    case vk::Format::eR32G32B32Uint:
    case vk::Format::eR32G32B32Sint:
    case vk::Format::eR32G32B32Sfloat:
        return 12;

    case vk::Format::eR32G32B32A32Uint:
    case vk::Format::eR32G32B32A32Sint:
    case vk::Format::eR32G32B32A32Sfloat:
        return 16;

    case vk::Format::eR64Uint:
    case vk::Format::eR64Sint:
    case vk::Format::eR64Sfloat:
        return 8;

    case vk::Format::eR64G64Uint:
    case vk::Format::eR64G64Sint:
    case vk::Format::eR64G64Sfloat:
        return 16;

    case vk::Format::eR64G64B64Uint:
    case vk::Format::eR64G64B64Sint:
    case vk::Format::eR64G64B64Sfloat:
        return 24;

    case vk::Format::eR64G64B64A64Uint:
    case vk::Format::eR64G64B64A64Sint:
    case vk::Format::eR64G64B64A64Sfloat:
        return 32;

    case vk::Format::eB10G11R11UfloatPack32:
    case vk::Format::eE5B9G9R9UfloatPack32:
        return 4;

    case vk::Format::eD16Unorm:
        return 2;

    case vk::Format::eX8D24UnormPack32:
        return 4;

    case vk::Format::eD32Sfloat:
        return 4;

    case vk::Format::eS8Uint:
        return 1;

    case vk::Format::eD16UnormS8Uint:
        return 3;

    case vk::Format::eD24UnormS8Uint:
        return 4;

    case vk::Format::eD32SfloatS8Uint:
        return 5;

    default:
        Assert(false);
        return 0;
    }
}

vk::ImageSubresourceLayers VulkanHelpers::GetSubresourceLayers(const vk::ImageSubresource &subresource)
{
    return vk::ImageSubresourceLayers(subresource.aspectMask, subresource.mipLevel, subresource.arrayLayer, 1);
}

vk::ImageSubresourceRange VulkanHelpers::GetSubresourceRange(const vk::ImageSubresource &subresource)
{
    return vk::ImageSubresourceRange(subresource.aspectMask, subresource.mipLevel, 1, subresource.arrayLayer, 1);
}

std::vector<vk::Framebuffer> VulkanHelpers::CreateSwapchainFramebuffers(const Device &device,
        const Swapchain &swapchain, const RenderPass &renderPass)
{
    const std::vector<vk::ImageView> &imageViews = swapchain.GetImageViews();
    const vk::Extent2D &extent = swapchain.GetExtent();

    std::vector<vk::Framebuffer> framebuffers;
    framebuffers.reserve(imageViews.size());

    for (const auto &imageView : imageViews)
    {
        const vk::FramebufferCreateInfo createInfo({}, renderPass.Get(),
                1, &imageView, extent.width, extent.height, 1);

        const auto [result, framebuffer] = device.Get().createFramebuffer(createInfo);
        Assert(result == vk::Result::eSuccess);

        framebuffers.push_back(framebuffer);
    }

    return framebuffers;
}

uint32_t VulkanHelpers::CalculateVertexStride(const VertexFormat &vertexFormat)
{
    uint32_t stride = 0;

    for (const auto &attribute : vertexFormat)
    {
        stride += GetFormatTexelSize(attribute);
    }

    return stride;
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

void VulkanHelpers::UpdateImageLayout(vk::Image image, const vk::ImageSubresourceRange &range,
        vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::CommandBuffer commandBuffer)
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
        Assert(false);
        break;
    }

    const vk::ImageMemoryBarrier imageMemoryBarrier(srcAccessMask, dstAccessMask,
            oldLayout, newLayout, 0, 0, image, range);

    commandBuffer.pipelineBarrier(srcStageMask, dstStageMask, {},
            0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
}

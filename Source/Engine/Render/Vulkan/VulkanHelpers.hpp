#pragma once

class Device;

namespace VulkanHelpers
{
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

    const vk::PipelineColorBlendAttachmentState kBlendStateDisabled(false);

    const vk::PipelineColorBlendAttachmentState kBlendStateAlphaBlend(true,
            vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd,
            vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd, kColorComponentFlagsRgba);

    bool IsDepthFormat(vk::Format format);

    vk::DeviceMemory AllocateDeviceMemory(const Device &device,
            vk::MemoryRequirements requirements, vk::MemoryPropertyFlags properties);

    void CopyToDeviceMemory(const Device &device, const uint8_t *src,
            vk::DeviceMemory memory, uint32_t memoryOffset, uint32_t size);

    uint32_t GetFormatSize(vk::Format format);
}

#pragma once

#include "Engine/Render/Vulkan/Device.hpp"

namespace VulkanConfig
{
    const std::vector<const char*> kRequiredExtensions = {};

    const std::vector<const char*> kRequiredDeviceExtensions{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
#ifndef __linux__
        VK_KHR_RAY_QUERY_EXTENSION_NAME,
#endif
    };

    constexpr Device::Features kRequiredDeviceFeatures{ .samplerAnisotropy = true,
        .accelerationStructure = true,
        .rayTracingPipeline = true,
        .descriptorIndexing = true,
        .bufferDeviceAddress = true,
        .scalarBlockLayout = true,
        .updateAfterBind = true,
#ifndef __linux__
        .rayQuery = true
#endif
    };

    const std::vector<vk::DescriptorPoolSize> kDescriptorPoolSizes{
        { vk::DescriptorType::eUniformBuffer, 2048 },
        { vk::DescriptorType::eCombinedImageSampler, 2048 },
        { vk::DescriptorType::eStorageImage, 2048 },
        { vk::DescriptorType::eAccelerationStructureKHR, 512 },
    };

    constexpr uint32_t kSwapchainMinImageCount = 3;

    constexpr uint32_t kMaxDescriptorSetCount = 512;

    constexpr std::optional<float> kMaxAnisotropy = 16.0f;
}

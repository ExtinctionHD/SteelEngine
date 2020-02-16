#pragma once

#include "Utils/Helpers.hpp"

namespace VulkanConfig
{
#ifdef NDEBUG
    constexpr bool kValidationEnabled = false;
#else
    constexpr bool kValidationEnabled = true;
#endif

    const std::vector<const char*> kRequiredExtensions;

    const std::vector<const char*> kRequiredDeviceExtensions
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_NV_RAY_TRACING_EXTENSION_NAME
    };

    constexpr DeviceFeatures kRequiredDeviceFeatures{ true };

    constexpr vk::DeviceSize kStagingBufferCapacity = Numbers::kGigabyte;

    const std::vector<vk::DescriptorPoolSize> kDescriptorPoolSizes{
        { vk::DescriptorType::eUniformBuffer, 256 },
        { vk::DescriptorType::eCombinedImageSampler, 256 }
    };

    constexpr uint32_t kMaxDescriptorSetCount = 256;
}

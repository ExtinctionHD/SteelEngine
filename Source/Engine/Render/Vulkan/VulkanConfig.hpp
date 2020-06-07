#pragma once

#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"

namespace VulkanConfig
{
#ifdef NDEBUG
    constexpr bool kValidationEnabled = false;
#else
    constexpr bool kValidationEnabled = true;
#endif

    const std::vector<const char*> kRequiredExtensions = {};

    const std::vector<const char*> kRequiredDeviceExtensions{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_EXTENSION_NAME,
        VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
    };

    constexpr Device::Features kRequiredDeviceFeatures{
        .samplerAnisotropy = true,
        .rayTracing = true,
        .descriptorIndexing = true,
        .bufferDeviceAddress = true
    };

    const std::vector<vk::DescriptorPoolSize> kDescriptorPoolSizes{
        { vk::DescriptorType::eUniformBuffer, 256 },
        { vk::DescriptorType::eCombinedImageSampler, 256 }
    };

    constexpr uint32_t kMaxDescriptorSetCount = 256;

    constexpr std::optional<float> kMaxAnisotropy = 16.0f;

    constexpr SamplerDescription kDefaultSamplerDescription{
        vk::Filter::eLinear,
        vk::Filter::eLinear,
        vk::SamplerMipmapMode::eLinear,
        vk::SamplerAddressMode::eRepeat,
        kMaxAnisotropy,
        0.0f, std::numeric_limits<float>::max()
    };
}

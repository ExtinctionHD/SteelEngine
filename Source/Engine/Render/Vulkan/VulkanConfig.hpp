#pragma once

struct DeviceFeatures
{
    uint32_t samplerAnisotropy : 1;
    uint32_t accelerationStructure : 1;
    uint32_t rayTracingPipeline : 1;
    uint32_t descriptorIndexing : 1;
    uint32_t bufferDeviceAddress : 1;
    uint32_t scalarBlockLayout : 1;
    uint32_t updateAfterBind : 1;
    uint32_t rayQuery : 1;
};

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
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
        VK_KHR_RAY_QUERY_EXTENSION_NAME,
    };

    constexpr DeviceFeatures kRequiredDeviceFeatures{
        .samplerAnisotropy = true,
        .accelerationStructure = true,
        .rayTracingPipeline = true,
        .descriptorIndexing = true,
        .bufferDeviceAddress = true,
        .scalarBlockLayout = true,
        .updateAfterBind = true,
        .rayQuery = true
    };

    const std::vector<vk::DescriptorPoolSize> kDescriptorPoolSizes{
        { vk::DescriptorType::eUniformBuffer, 2048 },
        { vk::DescriptorType::eCombinedImageSampler, 2048 },
        { vk::DescriptorType::eStorageImage, 2048 },
        { vk::DescriptorType::eAccelerationStructureKHR, 512 }
    };

    constexpr uint32_t kSwapchainMinImageCount = 3;

    constexpr uint32_t kMaxDescriptorSetCount = 512;
}

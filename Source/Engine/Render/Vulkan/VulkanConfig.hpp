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

    constexpr DeviceFeatures kRequiredDeviceFeatures{
        .samplerAnisotropy = true,
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
}

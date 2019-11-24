#pragma once

#include <vulkan/vulkan.hpp>

#include <vector>

class VulkanInstance
{
public:
    VulkanInstance() = delete;
    VulkanInstance(const VulkanInstance&) = delete;
    VulkanInstance(std::vector<const char*> requiredExtensions, bool validationEnabled);

    const vk::Instance& Get() const { return instance.get(); }

private:
    vk::UniqueInstance instance;
    vk::UniqueDebugReportCallbackEXT debugReportCallback;

    bool RequiredExtensionsSupported(const std::vector<const char *> &requiredExtensions) const;
    bool RequiredLayersSupported(const std::vector<const char*> &requiredLayers) const;

    void SetupDebugReportCallback();
};

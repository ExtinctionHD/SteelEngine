#pragma once

#include <vulkan/vulkan.hpp>

#include <vector>

class VulkanInstance
{
public:
    VulkanInstance() = delete;
    VulkanInstance(const VulkanInstance&) = delete;
    VulkanInstance(std::vector<const char*> requiredExtensions, bool validationEnabled);

    const vk::UniqueInstance& Get() const { return instance; }

private:
    vk::UniqueInstance instance;

    bool RequiredExtensionsSupported(const std::vector<const char *> &requiredExtensions) const;
    bool RequiredLayersSupported(const std::vector<const char*> &requiredLayers) const;
};

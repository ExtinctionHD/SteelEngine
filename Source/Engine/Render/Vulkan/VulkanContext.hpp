#pragma once

#include "Engine/Render/Vulkan/VukanInstance.hpp"

#include <memory>

class VulkanContext
{
public:
    static VulkanContext* Get();

private:
    inline static VulkanContext *vulkanContext = nullptr;

    VulkanContext();

    std::vector<const char*> GetRequiredExtensions() const;

    std::unique_ptr<VulkanInstance> vulkanInstance;
};

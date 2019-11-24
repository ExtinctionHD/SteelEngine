#pragma once

#include "Engine/Render/Vulkan/VukanInstance.hpp"
#include "Engine/Render/Vulkan/VulkanDevice.hpp"

#include <memory>

class VulkanContext
{
public:
    static VulkanContext *Get();

private:
    inline static VulkanContext *vulkanContext = nullptr;

    VulkanContext();

    std::unique_ptr<VulkanInstance> vulkanInstance;
    std::unique_ptr<VulkanDevice> vulkanDevice;
};

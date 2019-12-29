#pragma once

#include <memory>

#include "Engine/Render/Vulkan/VukanInstance.hpp"
#include "Engine/Render/Vulkan/VulkanDevice.hpp"
#include "Engine/Render/Vulkan/VulkanSurface.hpp"
#include "Engine/Render/Vulkan/VulkanSwapchain.hpp"

class Window;

class VulkanContext
{
public:
    VulkanContext(const Window &window);

private:
    std::shared_ptr<VulkanInstance> vulkanInstance;
    std::shared_ptr<VulkanDevice> vulkanDevice;

    std::unique_ptr<VulkanSurface> vulkanSurface;
    std::unique_ptr<VulkanSwapchain> vulkanSwapchain;
};

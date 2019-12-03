#pragma once

#include <memory>

#include "Engine/Render/Vulkan/VukanInstance.hpp"
#include "Engine/Render/Vulkan/VulkanDevice.hpp"
#include "Engine/Render/Vulkan/VulkanSurface.hpp"

struct GLFWwindow;

class VulkanContext
{
public:
    VulkanContext(GLFWwindow *window);

private:
    std::unique_ptr<VulkanInstance> vulkanInstance;
    std::unique_ptr<VulkanDevice> vulkanDevice;
    std::unique_ptr<VulkanSurface> vulkanSurface;
};

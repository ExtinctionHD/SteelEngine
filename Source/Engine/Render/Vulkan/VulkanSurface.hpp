#pragma once

#include "Engine/Render/Vulkan/VukanInstance.hpp"

struct GLFWwindow;

class VulkanSurface
{
public:
    static std::unique_ptr<VulkanSurface> Create(std::shared_ptr<VulkanInstance> instance, GLFWwindow *window);

    VulkanSurface(std::shared_ptr<VulkanInstance> aInstance, vk::SurfaceKHR aSurface);
    ~VulkanSurface();

    vk::SurfaceKHR Get() const { return surface; }

private:
    std::shared_ptr<VulkanInstance> instance;

    vk::SurfaceKHR surface;
};

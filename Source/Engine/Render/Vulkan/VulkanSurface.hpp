#pragma once

#include <vulkan/vulkan.hpp>

struct GLFWwindow;

class VulkanSurface
{
public:
    VulkanSurface() = delete;
    VulkanSurface(const VulkanSurface &) = delete;
    VulkanSurface(vk::Instance instance, GLFWwindow *window);

    vk::SurfaceKHR Get() const { return surface.get(); }

private:
    vk::UniqueSurfaceKHR surface;
};

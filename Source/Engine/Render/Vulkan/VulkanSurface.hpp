#pragma once

#include <vulkan/vulkan.hpp>

struct GLFWwindow;

class VulkanSurface
{
public:
    static std::unique_ptr<VulkanSurface> Create(vk::Instance instance, GLFWwindow *window);

    VulkanSurface(vk::SurfaceKHR aSurface);

    vk::SurfaceKHR Get() const { return surface.get(); }

private:
    vk::UniqueSurfaceKHR surface;
};

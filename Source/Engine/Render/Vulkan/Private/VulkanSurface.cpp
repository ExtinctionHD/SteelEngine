#include "Engine/Render/Vulkan/VulkanSurface.hpp"

#include "Utils/Assert.hpp"

#include <GLFW/glfw3.h>

std::unique_ptr<VulkanSurface> VulkanSurface::Create(vk::Instance instance, GLFWwindow *window)
{
    VkSurfaceKHR surface;
    Assert(glfwCreateWindowSurface(instance, window, nullptr, &surface) == VK_SUCCESS);

    return std::make_unique<VulkanSurface>(surface);
}

VulkanSurface::VulkanSurface(vk::SurfaceKHR aSurface)
    : surface(aSurface)
{}

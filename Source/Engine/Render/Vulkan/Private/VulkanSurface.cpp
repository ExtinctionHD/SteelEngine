#include "Engine/Render/Vulkan/VulkanSurface.hpp"

#include "Utils/Assert.hpp"

#include <GLFW/glfw3.h>

VulkanSurface::VulkanSurface(vk::Instance instance, GLFWwindow *window)
{
    VkSurfaceKHR vkSurface = surface.get();
    Assert(glfwCreateWindowSurface(instance, window, nullptr, &vkSurface) == VK_SUCCESS);
}

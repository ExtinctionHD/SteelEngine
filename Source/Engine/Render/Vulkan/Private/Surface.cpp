#include <GLFW/glfw3.h>

#include "Engine/Render/Vulkan/Surface.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Utils/Assert.hpp"


std::unique_ptr<Surface> Surface::Create(GLFWwindow* window)
{
    VkSurfaceKHR surface;
    Assert(glfwCreateWindowSurface(VulkanContext::instance->Get(), window, nullptr, &surface) == VK_SUCCESS);

    LogD << "Surface created" << "\n";

    return std::unique_ptr<Surface>(new Surface(surface));
}

Surface::Surface(vk::SurfaceKHR surface_)
    : surface(surface_)
{}

Surface::~Surface()
{
    VulkanContext::instance->Get().destroySurfaceKHR(surface);
}

#include <utility>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Engine/Render/Vulkan/VulkanSurface.hpp"

#include "Utils/Assert.hpp"


std::unique_ptr<VulkanSurface> VulkanSurface::Create(std::shared_ptr<VulkanInstance> instance, GLFWwindow *window)
{
    VkSurfaceKHR surface;
    Assert(glfwCreateWindowSurface(instance->Get(), window, nullptr, &surface) == VK_SUCCESS);

    return std::make_unique<VulkanSurface>(instance, surface);
}

VulkanSurface::VulkanSurface(std::shared_ptr<VulkanInstance> aInstance, vk::SurfaceKHR aSurface)
    : instance(std::move(aInstance))
    , surface(aSurface)
{}

VulkanSurface::~VulkanSurface()
{
    instance->Get().destroySurfaceKHR(surface);
}

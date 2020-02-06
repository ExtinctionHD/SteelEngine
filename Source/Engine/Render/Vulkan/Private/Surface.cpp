#include "Engine/Render/Vulkan/Surface.hpp"

#include "Utils/Assert.hpp"


std::unique_ptr<Surface> Surface::Create(std::shared_ptr<Instance> instance, GLFWwindow *window)
{
    VkSurfaceKHR surface;
    Assert(glfwCreateWindowSurface(instance->Get(), window, nullptr, &surface) == VK_SUCCESS);

    LogD << "Surface created" << "\n";

    return std::unique_ptr<Surface>(new Surface(instance, surface));
}

Surface::Surface(std::shared_ptr<Instance> aInstance, vk::SurfaceKHR aSurface)
    : instance(aInstance)
    , surface(aSurface)
{}

Surface::~Surface()
{
    instance->Get().destroySurfaceKHR(surface);
}

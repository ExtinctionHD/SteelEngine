#include <GLFW/glfw3.h>

#include "Engine/Render/Vulkan/VulkanContext.hpp"

namespace SVulkanContext
{
    std::vector<const char *> GetRequiredExtensions()
    {
        uint32_t count = 0;
        const char **extensions = glfwGetRequiredInstanceExtensions(&count);

        return std::vector<const char*>(extensions, extensions + count);
    }

    const std::vector<const char*> kRequiredDeviceExtensions
    {
        VK_NV_RAY_TRACING_EXTENSION_NAME
    };
}

VulkanContext::VulkanContext(GLFWwindow *window)
{
#ifdef NDEBUG
    const bool validationEnabled = false;
#else
    const bool validationEnabled = true;
#endif

    // TODO: dependencies between vulkan objects
    vulkanInstance = VulkanInstance::Create(SVulkanContext::GetRequiredExtensions(), validationEnabled);
    vulkanSurface = VulkanSurface::Create(vulkanInstance->Get(), window);
    vulkanDevice = VulkanDevice::Create(vulkanInstance->Get(), vulkanSurface->Get(), SVulkanContext::kRequiredDeviceExtensions);
}

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
    const VulkanInstance::eValidation validation = VulkanInstance::eValidation::kDisabled;
#else
    const VulkanInstance::eValidation validation = VulkanInstance::eValidation::kEnabled;
#endif

    vulkanInstance = VulkanInstance::Create(SVulkanContext::GetRequiredExtensions(), validation);
    vulkanSurface = VulkanSurface::Create(vulkanInstance, window);
    vulkanDevice = VulkanDevice::Create(vulkanInstance, vulkanSurface->Get(), SVulkanContext::kRequiredDeviceExtensions);
}

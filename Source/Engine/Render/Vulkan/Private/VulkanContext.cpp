#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include <GLFW/glfw3.h>

namespace SVulkanContext
{
    std::vector<const char *> GetRequiredExtensions()
    {
        uint32_t count = 0;
        const char** extensions = glfwGetRequiredInstanceExtensions(&count);

        return std::vector<const char*>(extensions, extensions + count);
    }

    const std::vector<const char*> kRequiredDeviceExtensions
    {
        VK_NV_RAY_TRACING_EXTENSION_NAME
    };
}

VulkanContext *VulkanContext::Get()
{
    if (vulkanContext == nullptr)
    {
        vulkanContext = new VulkanContext();
    }

    return vulkanContext;
}

VulkanContext::VulkanContext()
{
#ifdef NDEBUG
    const bool validationEnabled = false;
#else
    const bool validationEnabled = true;
#endif

    vulkanInstance = std::make_unique<VulkanInstance>(SVulkanContext::GetRequiredExtensions(), validationEnabled);
    vulkanDevice = std::make_unique<VulkanDevice>(vulkanInstance->Get(), SVulkanContext::kRequiredDeviceExtensions);
}


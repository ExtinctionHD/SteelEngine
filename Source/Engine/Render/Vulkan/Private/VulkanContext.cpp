#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include <GLFW/glfw3.h>

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

    vulkanInstance = std::make_unique<VulkanInstance>(GetRequiredExtensions(), validationEnabled);
}

std::vector<const char *> VulkanContext::GetRequiredExtensions() const
{
    uint32_t count = 0;
    const char **extensions = glfwGetRequiredInstanceExtensions(&count);

    return std::vector<const char*>(extensions, extensions + count);
}


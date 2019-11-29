#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Engine/Render/Vulkan/VukanInstance.hpp"
#include "Engine/Render/Vulkan/VulkanDevice.hpp"
#include "Engine/Render/Vulkan/VulkanSurface.hpp"
#include "Utils/Assert.hpp"

#include <GLFW/glfw3.h>

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

void VulkanContext::Initialize()
{
    vulkanContext = new VulkanContext();
}

VulkanContext *VulkanContext::Get()
{
    if (vulkanContext == nullptr)
    {
        Initialize();
    }

    return vulkanContext;
}

void VulkanContext::CreateSurface(GLFWwindow *window)
{
    Assert(vulkanInstance != nullptr);

    vulkanSurface = VulkanSurface::Create(vulkanInstance->Get(), window);
}

VulkanContext::VulkanContext()
{
#ifdef NDEBUG
    const bool validationEnabled = false;
#else
    const bool validationEnabled = true;
#endif

    vulkanInstance = VulkanInstance::Create(SVulkanContext::GetRequiredExtensions(), validationEnabled);
    vulkanDevice = VulkanDevice::Create(vulkanInstance->Get(), SVulkanContext::kRequiredDeviceExtensions);
}

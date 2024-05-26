#include <GLFW/glfw3.h>

#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Engine/Render/Vulkan/VulkanConfig.hpp"
#include "Engine/ConsoleVariable.hpp"
#include "Engine/Window.hpp"

namespace Details
{
#ifdef NDEBUG
    static bool validationEnabled = false;
#else
    static bool validationEnabled = true;
#endif
    static CVarBool validationEnabledCVar("vk.ValidationEnabled", validationEnabled, CVarFlagBits::eReadOnly);


    static void InitializeDefaultDispatcher()
    {
        const vk::DynamicLoader dynamicLoader;
        const PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr
                = dynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");

        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
    }

    static std::vector<const char*> UpdateRequiredExtensions(
            const std::vector<const char*>& requiredExtension)
    {
        uint32_t count = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&count);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + count);
        extensions.reserve(extensions.size() + requiredExtension.size());

        std::ranges::copy(requiredExtension, std::back_inserter(extensions));

        return extensions;
    }
}

std::unique_ptr<Instance> VulkanContext::instance;

std::unique_ptr<Device> VulkanContext::device;
std::unique_ptr<Surface> VulkanContext::surface;
std::unique_ptr<Swapchain> VulkanContext::swapchain;

std::unique_ptr<DescriptorManager> VulkanContext::descriptorManager;
std::unique_ptr<ShaderManager> VulkanContext::shaderManager;
std::unique_ptr<MemoryManager> VulkanContext::memoryManager;

void VulkanContext::Create(const Window& window)
{
    EASY_FUNCTION()

    Details::InitializeDefaultDispatcher();

    const std::vector<const char*> requiredExtensions
            = Details::UpdateRequiredExtensions(VulkanConfig::kRequiredExtensions);

    instance = Instance::Create(requiredExtensions);
    surface = Surface::Create(window.Get());
    device = Device::Create(VulkanConfig::kRequiredDeviceFeatures, VulkanConfig::kRequiredDeviceExtensions);
    swapchain = Swapchain::Create(window.GetExtent());

    descriptorManager = DescriptorManager::Create();

    shaderManager = std::make_unique<ShaderManager>();
    memoryManager = std::make_unique<MemoryManager>();
}

void VulkanContext::Destroy()
{
    memoryManager.reset();
    shaderManager.reset();
    descriptorManager.reset();
    swapchain.reset();
    device.reset();
    surface.reset();
    instance.reset();
}

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

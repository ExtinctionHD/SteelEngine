#include "Engine/Render/Vulkan/VukanInstance.hpp"

#include "Engine/Render/Vulkan/VulkanEXT.h"

#include "Utils/Assert.hpp"
#include "Utils/Logger.hpp"

namespace SVulkanInstance
{
    bool RequiredExtensionsSupported(const std::vector<const char*> &requiredExtensions)
    {
        const auto extensions = vk::enumerateInstanceExtensionProperties();

        for (const auto &requiredExtension : requiredExtensions)
        {
            const auto pred = [&requiredExtension](const auto &extension)
                {
                    return strcmp(extension.extensionName, requiredExtension) == 0;
                };

            const auto it = std::find_if(extensions.begin(), extensions.end(), pred);

            if (it == extensions.end())
            {
                LogE << "Required extension not found: " << requiredExtension << "\n";
                return false;
            }
        }

        return true;
    }

    bool RequiredLayersSupported(const std::vector<const char*> &requiredLayers)
    {
        const auto layers = vk::enumerateInstanceLayerProperties();

        for (const auto &requiredLayer : requiredLayers)
        {
            const auto pred = [&requiredLayer](const auto &layer)
                {
                    return strcmp(layer.layerName, requiredLayer) == 0;
                };

            const auto it = std::find_if(layers.begin(), layers.end(), pred);

            if (it == layers.end())
            {
                LogE << "Required layer not found: " << requiredLayer << "\n";
                return false;
            }
        }

        return true;
    }

    VkBool32 VulkanDebugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *)
    {
        std::string type;
        if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
        {
            type = "Validation";
        }
        else if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
        {
            type = "Performance";
        }
        else
        {
            type = "General";
        }

        std::string severity;
        switch (messageSeverity)
        {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            severity = "error";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            severity = "warning";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            severity = "verbose";
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            severity = "info";
            break;
        default:
            break;
        }

        std::cout << "[VULKAN]\t" << type << " " << severity << ": " << pCallbackData->pMessage << "/n";

        return false;
    }
}

VulkanInstance::VulkanInstance(std::vector<const char *> requiredExtensions, bool validationEnabled)
{
    std::vector<const char*> requiredLayers;

    if (validationEnabled)
    {
        requiredExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        requiredLayers.emplace_back("VK_LAYER_LUNARG_standard_validation");
    }

    Assert(SVulkanInstance::RequiredExtensionsSupported(requiredExtensions)
        && SVulkanInstance::RequiredLayersSupported(requiredLayers));

    vk::ApplicationInfo appInfo("VulkanRayTracing", 1, "VRTEngine", 1, VK_API_VERSION_1_1);

    const vk::InstanceCreateInfo createInfo({}, &appInfo, static_cast<uint32_t>(requiredLayers.size()),
        requiredLayers.data(), static_cast<uint32_t>(requiredExtensions.size()), requiredExtensions.data());

    instance = createInstanceUnique(createInfo);

    if (validationEnabled)
    {
        vkExtInitInstance(instance.get());

        SetupDebugUtilsMessenger();
    }
}

void VulkanInstance::SetupDebugUtilsMessenger()
{
    const vk::DebugUtilsMessageSeverityFlagsEXT severity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose;

    const vk::DebugUtilsMessageTypeFlagsEXT type = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
            | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;

    const vk::DebugUtilsMessengerCreateInfoEXT createInfo({}, severity, type,
        SVulkanInstance::VulkanDebugUtilsMessengerCallback);

    debugUtilsMessenger = instance->createDebugUtilsMessengerEXTUnique(createInfo);
}

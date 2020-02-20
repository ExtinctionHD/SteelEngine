#include <VulkanExtensions/VulkanExtensions.h>

#include "Engine/Render/Vulkan/Instance.hpp"
#include "Engine/Render/Vulkan/VulkanConfig.hpp"
#include "Engine/Config.hpp"

#include "Utils/Assert.hpp"
#include "Utils/Logger.hpp"

namespace SInstance
{
    bool RequiredExtensionsSupported(const std::vector<const char*> &requiredExtensions)
    {
        const auto [result, extensions] = vk::enumerateInstanceExtensionProperties();
        Assert(result == vk::Result::eSuccess);

        for (const auto &requiredExtension : requiredExtensions)
        {
            const auto pred = [&requiredExtension](const auto &extension)
                {
                    return std::strcmp(extension.extensionName, requiredExtension) == 0;
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
        const auto [result, layers] = vk::enumerateInstanceLayerProperties();
        Assert(result == vk::Result::eSuccess);

        for (const auto &requiredLayer : requiredLayers)
        {
            const auto pred = [&requiredLayer](const auto &layer)
                {
                    return std::strcmp(layer.layerName, requiredLayer) == 0;
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
            VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
            void *)
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

        std::string message(pCallbackData->pMessage);
        message = message.substr(0, message.find("(http")); // remove link to vulkan docs

        std::cout << "[VULKAN] " << type << " " << severity << ": " << message << "\n";

        return false;
    }

    vk::DebugUtilsMessengerEXT CreateDebugUtilsMessenger(vk::Instance instance)
    {
        const vk::DebugUtilsMessageSeverityFlagsEXT severity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
                | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::
                eVerbose;

        const vk::DebugUtilsMessageTypeFlagsEXT type = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
                | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::
                eValidation;

        const vk::DebugUtilsMessengerCreateInfoEXT createInfo({}, severity, type,
                VulkanDebugUtilsMessengerCallback);

        const auto [result, debugUtilsMessenger] = instance.createDebugUtilsMessengerEXT(createInfo);
        Assert(result == vk::Result::eSuccess);

        return debugUtilsMessenger;
    }
}

std::shared_ptr<Instance> Instance::Create(std::vector<const char*> requiredExtensions)
{
    std::vector<const char*> requiredLayers;

    if (VulkanConfig::kValidationEnabled)
    {
        requiredExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        requiredLayers.emplace_back("VK_LAYER_LUNARG_standard_validation");
    }

    Assert(SInstance::RequiredExtensionsSupported(requiredExtensions)
            && SInstance::RequiredLayersSupported(requiredLayers));

    vk::ApplicationInfo appInfo(Config::kEngineName, 1, Config::kEngineName, 1, VK_API_VERSION_1_1);

    const vk::InstanceCreateInfo createInfo({}, &appInfo, static_cast<uint32_t>(requiredLayers.size()),
            requiredLayers.data(), static_cast<uint32_t>(requiredExtensions.size()), requiredExtensions.data());

    const auto [result, instance] = createInstance(createInfo);
    Assert(result == vk::Result::eSuccess);

    vkExtInitInstance(instance);

    vk::DebugUtilsMessengerEXT debugUtilsMessenger;
    if (VulkanConfig::kValidationEnabled)
    {
        debugUtilsMessenger = SInstance::CreateDebugUtilsMessenger(instance);

        LogI << "Validation enabled" << "\n";
    }

    LogD << "Instance created" << "\n";

    return std::shared_ptr<Instance>(new Instance(instance, debugUtilsMessenger));
}

Instance::Instance(vk::Instance aInstance, vk::DebugUtilsMessengerEXT aDebugUtilsMessenger)
    : instance(aInstance)
    , debugUtilsMessenger(aDebugUtilsMessenger)
{}

Instance::~Instance()
{
    if (debugUtilsMessenger)
    {
        instance.destroyDebugUtilsMessengerEXT(debugUtilsMessenger);
    }

    instance.destroy();
}

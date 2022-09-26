#include "Engine/Render/Vulkan/Instance.hpp"

#include "Engine/Render/Vulkan/VulkanConfig.hpp"
#include "Engine/Config.hpp"

#include "Utils/Assert.hpp"
#include "Utils/Logger.hpp"

namespace Details
{
    static bool RequiredExtensionsSupported(const std::vector<const char*>& requiredExtensions)
    {
        const auto [result, extensions] = vk::enumerateInstanceExtensionProperties();
        Assert(result == vk::Result::eSuccess);

        for (const auto& requiredExtension : requiredExtensions)
        {
            const auto pred = [&requiredExtension](const auto& extension)
                {
                    return std::strcmp(extension.extensionName, requiredExtension) == 0;
                };

            const auto it = std::ranges::find_if(extensions, pred);

            if (it == extensions.end())
            {
                LogE << "Required extension not found: " << requiredExtension << "\n";
                return false;
            }
        }

        return true;
    }

    static bool RequiredLayersSupported(const std::vector<const char*>& requiredLayers)
    {
        const auto [result, layers] = vk::enumerateInstanceLayerProperties();
        Assert(result == vk::Result::eSuccess);

        for (const auto& requiredLayer : requiredLayers)
        {
            const auto pred = [&requiredLayer](const auto& layer)
                {
                    return std::strcmp(layer.layerName, requiredLayer) == 0;
                };

            const auto it = std::ranges::find_if(layers, pred);

            if (it == layers.end())
            {
                LogE << "Required layer not found: " << requiredLayer << "\n";
                return false;
            }
        }

        return true;
    }

    static VkBool32 VulkanDebugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT,
            VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void*)
    {
        std::string message(pCallbackData->pMessage);
        message = message.substr(0, message.find("(http")); // remove link to vulkan docs

        std::cout << "[VULKAN] " << message << "\n";

        Assert(false);

        return false;
    }

    vk::DebugUtilsMessengerEXT CreateDebugUtilsMessenger(vk::Instance instance)
    {
        const vk::DebugUtilsMessageSeverityFlagsEXT severity
                = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
                | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;

        const vk::DebugUtilsMessageTypeFlagsEXT type
                = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
                | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
                | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;

        const vk::DebugUtilsMessengerCreateInfoEXT createInfo({}, severity, type,
                VulkanDebugUtilsMessengerCallback);

        const auto [result, debugUtilsMessenger] = instance.createDebugUtilsMessengerEXT(createInfo);
        Assert(result == vk::Result::eSuccess);

        return debugUtilsMessenger;
    }
}

std::unique_ptr<Instance> Instance::Create(std::vector<const char*> requiredExtensions)
{
    std::vector<const char*> requiredLayers;

    if constexpr (VulkanConfig::kValidationEnabled)
    {
        requiredExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        requiredLayers.emplace_back("VK_LAYER_KHRONOS_validation");
    }

    Assert(Details::RequiredExtensionsSupported(requiredExtensions)
            && Details::RequiredLayersSupported(requiredLayers));

    const vk::ApplicationInfo appInfo(Config::kEngineName, 1, Config::kEngineName, 1, VK_API_VERSION_1_2);

    const vk::InstanceCreateInfo createInfo({}, &appInfo, static_cast<uint32_t>(requiredLayers.size()),
            requiredLayers.data(), static_cast<uint32_t>(requiredExtensions.size()), requiredExtensions.data());

    const auto [result, instance] = vk::createInstance(createInfo);
    Assert(result == vk::Result::eSuccess);

    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);

    vk::DebugUtilsMessengerEXT debugUtilsMessenger;
    if constexpr (VulkanConfig::kValidationEnabled)
    {
        debugUtilsMessenger = Details::CreateDebugUtilsMessenger(instance);

        LogI << "Validation enabled" << "\n";
    }

    LogD << "Instance created" << "\n";

    return std::unique_ptr<Instance>(new Instance(instance, debugUtilsMessenger));
}

Instance::Instance(vk::Instance instance_, vk::DebugUtilsMessengerEXT debugUtilsMessenger_)
    : instance(instance_)
    , debugUtilsMessenger(debugUtilsMessenger_)
{}

Instance::~Instance()
{
    if (debugUtilsMessenger)
    {
        instance.destroyDebugUtilsMessengerEXT(debugUtilsMessenger);
    }

    instance.destroy();
}

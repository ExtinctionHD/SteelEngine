#include "Engine/Render/Vulkan/VukanInstance.hpp"

#include "Engine/Render/Vulkan/VulkanEXT.h"

#include "Utils/Assert.hpp"
#include "Utils/Logger.hpp"

namespace  SVulkanInstance
{
    VkBool32 VulkanDebugReportCallback(VkDebugReportFlagsEXT, VkDebugReportObjectTypeEXT,
            uint64_t, size_t, int32_t, const char *, const char *msg, void*)
    {
        LogE << "[VULKAN] " << msg << "/n";
        return false;
    }
}

VulkanInstance::VulkanInstance(std::vector<const char *> requiredExtensions, bool validationEnabled)
{
    std::vector<const char*> requiredLayers;

    if (validationEnabled)
    {
        requiredExtensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        requiredLayers.emplace_back("VK_LAYER_LUNARG_standard_validation");
    }

    AssertD(RequiredExtensionsSupported(requiredExtensions) && RequiredLayersSupported(requiredLayers));

    vk::ApplicationInfo appInfo("VulkanRayTracing", 1, "VRTEngine", 1, VK_API_VERSION_1_1);

    const vk::InstanceCreateInfo createInfo({}, &appInfo, static_cast<uint32_t>(requiredLayers.size()), requiredLayers.data(),
            static_cast<uint32_t>(requiredExtensions.size()), requiredExtensions.data());

    instance = vk::createInstanceUnique(createInfo);

    if (validationEnabled)
    {
        vkExtInitInstance(instance.get());

        SetupDebugReportCallback();
    }
}

bool VulkanInstance::RequiredExtensionsSupported(const std::vector<const char *> &requiredExtensions) const
{
    auto extensions = vk::enumerateInstanceExtensionProperties();
    for (const auto& requiredExtension : requiredExtensions)
    {
        const auto pred = [&requiredExtension](const auto& extension)
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

bool VulkanInstance::RequiredLayersSupported(const std::vector<const char *> &requiredLayers) const
{
    auto layers = vk::enumerateInstanceLayerProperties();
    for (const auto& requiredLayer : requiredLayers)
    {
        const auto pred = [&requiredLayer](const auto& layer)
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

void VulkanInstance::SetupDebugReportCallback()
{
    const vk::DebugReportFlagsEXT allFlags = static_cast<vk::DebugReportFlagsEXT>(vk::FlagTraits<vk::DebugReportFlagBitsEXT>::allFlags);
    const vk::DebugReportCallbackCreateInfoEXT createInfo(allFlags, &SVulkanInstance::VulkanDebugReportCallback);

    debugReportCallback = instance->createDebugReportCallbackEXTUnique(createInfo, nullptr, {});
}



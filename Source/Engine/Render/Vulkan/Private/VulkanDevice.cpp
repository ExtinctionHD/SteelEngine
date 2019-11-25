#include "Engine/Render/Vulkan/VulkanDevice.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Utils/Logger.hpp"
#include "Utils/Assert.hpp"

namespace SVulkanDevice
{
    bool RequiredDeviceExtensionsSupported(vk::PhysicalDevice physicalDevice,
        const std::vector<const char*> &requiredDeviceExtensions)
    {
        const auto deviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();

        for (const auto &requiredDeviceExtension : requiredDeviceExtensions)
        {
            const auto pred = [&requiredDeviceExtension](const auto &extension)
                {
                    return strcmp(extension.extensionName, requiredDeviceExtension) == 0;
                };

            const auto it = std::find_if(deviceExtensions.begin(), deviceExtensions.end(), pred);

            if (it == deviceExtensions.end())
            {
                LogE << "Required device extension not found: " << requiredDeviceExtension << "\n";
                return false;
            }
        }

        return true;
    }

    bool IsSuitablePhysicalDevice(vk::PhysicalDevice physicalDevice,
        const std::vector<const char*> &requiredDeviceExtensions)
    {
        return RequiredDeviceExtensionsSupported(physicalDevice, requiredDeviceExtensions);
    }

    uint32_t FindGraphicsQueueIndex(vk::PhysicalDevice physicalDevice)
    {
        const auto queueFamilies = physicalDevice.getQueueFamilyProperties();

        const auto pred = [](const vk::QueueFamilyProperties &queueFamily)
            {
                return queueFamily.queueCount > 0 && queueFamily.queueFlags & vk::QueueFlagBits::eGraphics;
            };

        const auto it = std::find_if(queueFamilies.begin(), queueFamilies.end(), pred);

        Assert(it != queueFamilies.end());

        return static_cast<uint32_t>(std::distance(queueFamilies.begin(), it));
    }
}

VulkanDevice::VulkanDevice(vk::Instance instance, const std::vector<const char*> &requiredDeviceExtensions)
{
    const auto physicalDevices = instance.enumeratePhysicalDevices();

    const auto pred = [&requiredDeviceExtensions](const auto &physicalDevice)
        {
            return SVulkanDevice::IsSuitablePhysicalDevice(physicalDevice, requiredDeviceExtensions);
        };

    const auto it = std::find_if(physicalDevices.begin(), physicalDevices.end(), pred);

    if (it != physicalDevices.end())
    {
        const vk::PhysicalDevice physicalDevice = *it;
        const vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();

        const uint32_t queueIndex = SVulkanDevice::FindGraphicsQueueIndex(physicalDevice);
        const float queuePriority = 0.0;
        const vk::DeviceQueueCreateInfo queueCreateInfo({}, queueIndex, 1, &queuePriority);

        const vk::DeviceCreateInfo createInfo({}, 1, &queueCreateInfo, 0, nullptr,
            static_cast<uint32_t>(requiredDeviceExtensions.size()),
            requiredDeviceExtensions.data(), nullptr);

        device = physicalDevice.createDeviceUnique(createInfo, nullptr);

        LogI << "Physical device: " << properties.deviceName << "\n";
    }
    else
    {
        LogE << "Failed to find suitable physical device";
    }
}

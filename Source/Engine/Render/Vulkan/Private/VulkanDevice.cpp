#include <optional>

#include "Engine/Render/Vulkan/VulkanDevice.hpp"

#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Utils/Logger.hpp"
#include "Utils/Assert.hpp"

namespace SVulkanDevice
{
    bool RequiredDeviceExtensionsSupported(vk::PhysicalDevice physicalDevice,
        const std::vector<const char*> &requiredDeviceExtensions)
    {
        const auto [result, deviceExtensions] = physicalDevice.enumerateDeviceExtensionProperties();

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

    vk::PhysicalDevice ObtainSuitablePhysicalDevice(vk::Instance instance,
        const std::vector<const char*>& requiredDeviceExtensions)
    {
        const auto [result, physicalDevices] = instance.enumeratePhysicalDevices();
        Assert(result == vk::Result::eSuccess);

        const auto pred = [&requiredDeviceExtensions](const auto& physicalDevice)
        {
            return SVulkanDevice::IsSuitablePhysicalDevice(physicalDevice, requiredDeviceExtensions);
        };

        const auto it = std::find_if(physicalDevices.begin(), physicalDevices.end(), pred);
        Assert(it != physicalDevices.end());

        return *it;
    }

    uint32_t FindGraphicsQueueFamilyIndex(vk::PhysicalDevice physicalDevice)
    {
        const auto queueFamilies = physicalDevice.getQueueFamilyProperties();

        const auto pred = [](const vk::QueueFamilyProperties& queueFamily)
        {
            return queueFamily.queueCount > 0 && queueFamily.queueFlags & vk::QueueFlagBits::eGraphics;
        };

        const auto it = std::find_if(queueFamilies.begin(), queueFamilies.end(), pred);

        Assert(it != queueFamilies.end());

        return static_cast<uint32_t>(std::distance(queueFamilies.begin(), it));
    }

    std::optional<uint32_t> FindGraphicsAndPresetQueueFamilyIndex(
        vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
    {
        const auto queueFamilies = physicalDevice.getQueueFamilyProperties();

        for (uint32_t i = 0; i < queueFamilies.size(); ++i)
        {
            const auto [result, supportSurface] = physicalDevice.getSurfaceSupportKHR(i, surface);
            Assert(result == vk::Result::eSuccess);

            const bool suitableQueueFamily = queueFamilies[i].queueCount > 0
                && queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics && supportSurface;

            if (suitableQueueFamily)
            {
                return std::make_optional<uint32_t>(i);
            }
        }

        return std::nullopt;
    }

    std::optional<uint32_t> FindPresentQueueFamilyIndex(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
    {
        const auto queueFamilies = physicalDevice.getQueueFamilyProperties();

        for (uint32_t i = 0; i < queueFamilies.size(); ++i)
        {
            const auto [result, supportSurface] = physicalDevice.getSurfaceSupportKHR(i, surface);
            Assert(result == vk::Result::eSuccess);

            if (queueFamilies[i].queueCount > 0 && supportSurface)
            {
                return std::make_optional<uint32_t>(i);
            }
        }

        return std::nullopt;
    }

    std::vector<vk::DeviceQueueCreateInfo> ObtainQueueCreateInfos(
        vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
    {
        static const float queuePriority = 0.0;

        const uint32_t graphicsQueueFamilyIndex = SVulkanDevice::FindGraphicsQueueFamilyIndex(physicalDevice);

        const auto [result, supportSurface] = physicalDevice.getSurfaceSupportKHR(graphicsQueueFamilyIndex, surface);
        Assert(result == vk::Result::eSuccess);

        if (supportSurface)
        {
            return { vk::DeviceQueueCreateInfo({}, graphicsQueueFamilyIndex, 1, &queuePriority) };
        }

        const std::optional<uint32_t> commonQueueFamilyIndex
            = FindGraphicsAndPresetQueueFamilyIndex(physicalDevice, surface);

        if (commonQueueFamilyIndex.has_value())
        {
            return { vk::DeviceQueueCreateInfo({}, commonQueueFamilyIndex.value(), 1, &queuePriority) };
        }

        const std::optional<uint32_t> presentQueueFamilyIndex = FindPresentQueueFamilyIndex(physicalDevice, surface);
        Assert(presentQueueFamilyIndex.has_value());

        std::vector<vk::DeviceQueueCreateInfo> createInfos{
            vk::DeviceQueueCreateInfo({}, graphicsQueueFamilyIndex, 1, &queuePriority),
            vk::DeviceQueueCreateInfo({}, presentQueueFamilyIndex.value(), 1, &queuePriority)
        };

        return createInfos;
    }
}

std::unique_ptr<VulkanDevice> VulkanDevice::Create(vk::Instance instance, vk::SurfaceKHR surface,
    const std::vector<const char *> &requiredDeviceExtensions)
{
    const auto physicalDevice = SVulkanDevice::ObtainSuitablePhysicalDevice(instance, requiredDeviceExtensions);

    const std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos
        = SVulkanDevice::ObtainQueueCreateInfos(physicalDevice, surface);

    const vk::DeviceCreateInfo createInfo({},
        static_cast<uint32_t>(queueCreateInfos.size()), queueCreateInfos.data(),
        0, nullptr,
        static_cast<uint32_t>(requiredDeviceExtensions.size()), requiredDeviceExtensions.data(),
        nullptr);

    const auto [result, device] = physicalDevice.createDevice(createInfo);
    Assert(result == vk::Result::eSuccess);

    const vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
    LogI << "Physical device: " << properties.deviceName << "\n";

    return std::make_unique<VulkanDevice>(device);
}

VulkanDevice::VulkanDevice(vk::Device aDevice)
    : device(aDevice)
{}

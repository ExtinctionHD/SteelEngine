#include <optional>
#include <utility>

#include "Engine/Render/Vulkan/Device.hpp"
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
            const std::vector<const char*> &requiredDeviceExtensions)
    {
        const auto [result, physicalDevices] = instance.enumeratePhysicalDevices();
        Assert(result == vk::Result::eSuccess);

        const auto pred = [&requiredDeviceExtensions](const auto &physicalDevice)
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

        const auto pred = [](const vk::QueueFamilyProperties &queueFamily)
            {
                return queueFamily.queueCount > 0 && queueFamily.queueFlags & vk::QueueFlagBits::eGraphics;
            };

        const auto it = std::find_if(queueFamilies.begin(), queueFamilies.end(), pred);

        Assert(it != queueFamilies.end());

        return static_cast<uint32_t>(std::distance(queueFamilies.begin(), it));
    }

    std::optional<uint32_t> FindCommonQueueFamilyIndex(
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

    Device::QueuesProperties ObtainQueuesProperties(
            vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
    {
        const uint32_t graphicsQueueFamilyIndex = FindGraphicsQueueFamilyIndex(physicalDevice);

        const auto [result, supportSurface] = physicalDevice.getSurfaceSupportKHR(graphicsQueueFamilyIndex, surface);
        Assert(result == vk::Result::eSuccess);

        if (supportSurface)
        {
            return { graphicsQueueFamilyIndex, graphicsQueueFamilyIndex };
        }

        const std::optional<uint32_t> commonQueueFamilyIndex
                = FindCommonQueueFamilyIndex(physicalDevice, surface);

        if (commonQueueFamilyIndex.has_value())
        {
            return { graphicsQueueFamilyIndex, graphicsQueueFamilyIndex };
        }

        const std::optional<uint32_t> presentQueueFamilyIndex = FindPresentQueueFamilyIndex(physicalDevice, surface);
        Assert(presentQueueFamilyIndex.has_value());

        return { graphicsQueueFamilyIndex, presentQueueFamilyIndex.value() };
    }

    std::vector<vk::DeviceQueueCreateInfo> ObtainQueueCreateInfos(
            const Device::QueuesProperties &queuesProperties)
    {
        static const float queuePriority = 0.0;

        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos{
            vk::DeviceQueueCreateInfo({}, queuesProperties.graphicsFamilyIndex, 1, &queuePriority)
        };

        if (!queuesProperties.CommonFamily())
        {
            queueCreateInfos.emplace_back(vk::DeviceQueueCreateFlags(),
                    queuesProperties.presentFamilyIndex, 1, &queuePriority);
        }

        return queueCreateInfos;
    }
}

bool Device::QueuesProperties::CommonFamily() const
{
    return graphicsFamilyIndex == presentFamilyIndex;
}

std::vector<uint32_t> Device::QueuesProperties::GetUniqueIndices() const
{
    if (CommonFamily())
    {
        return { graphicsFamilyIndex };
    }

    return { graphicsFamilyIndex, presentFamilyIndex };
}

std::shared_ptr<Device> Device::Create(std::shared_ptr<Instance> instance, vk::SurfaceKHR surface,
        const std::vector<const char *> &requiredDeviceExtensions)
{
    const auto physicalDevice = SVulkanDevice::ObtainSuitablePhysicalDevice(instance->Get(), requiredDeviceExtensions);

    const QueuesProperties queuesProperties = SVulkanDevice::ObtainQueuesProperties(physicalDevice, surface);

    const std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos
            = SVulkanDevice::ObtainQueueCreateInfos(queuesProperties);

    const vk::DeviceCreateInfo createInfo({},
            static_cast<uint32_t>(queueCreateInfos.size()), queueCreateInfos.data(),
            0, nullptr,
            static_cast<uint32_t>(requiredDeviceExtensions.size()), requiredDeviceExtensions.data(),
            nullptr);

    const auto [result, device] = physicalDevice.createDevice(createInfo);
    Assert(result == vk::Result::eSuccess);

    const vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
    LogI << "GPU selected: " << properties.deviceName << "\n";

    LogD << "Device created" << "\n";

    return std::make_shared<Device>(instance, device, physicalDevice, queuesProperties);
}

Device::Device(std::shared_ptr<Instance> aInstance, vk::Device aDevice,
        vk::PhysicalDevice aPhysicalDevice, const QueuesProperties &aQueuesProperties)
    : instance(std::move(aInstance))
    , device(aDevice)
    , physicalDevice(aPhysicalDevice)
    , queuesProperties(aQueuesProperties)
{}

Device::~Device()
{
    device.destroy();
}

vk::SurfaceCapabilitiesKHR Device::GetSurfaceCapabilities(vk::SurfaceKHR surface) const
{
    const auto [result, capabilities] = physicalDevice.getSurfaceCapabilitiesKHR(surface);
    Assert(result == vk::Result::eSuccess);

    return capabilities;
}

std::vector<vk::SurfaceFormatKHR> Device::GetSurfaceFormats(vk::SurfaceKHR surface) const
{
    const auto [result, formats] = physicalDevice.getSurfaceFormatsKHR(surface);
    Assert(result == vk::Result::eSuccess);

    return formats;
}

const Device::QueuesProperties &Device::GetQueueProperties() const
{
    return queuesProperties;
}

uint32_t Device::GetMemoryTypeIndex(uint32_t typeBits, vk::MemoryPropertyFlags requiredProperties) const
{
    const vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties();

    std::optional<uint32_t> index;
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
        const vk::MemoryPropertyFlags currentTypeProperties = memoryProperties.memoryTypes[i].propertyFlags;
        const bool meetsRequirements = (currentTypeProperties & requiredProperties) == requiredProperties;
        const bool suitableType = typeBits & (1 << i);

        if (suitableType && meetsRequirements)
        {
            index = std::make_optional(i);
            break;
        }
    }

    Assert(index.has_value());

    return index.value();
}
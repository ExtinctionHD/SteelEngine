#pragma once

#include "Engine/Render/Vulkan/VukanInstance.hpp"

class VulkanDevice
{
public:
    struct QueuesProperties
    {
        uint32_t graphicsFamilyIndex;
        uint32_t presentFamilyIndex;

        bool CommonQueueFamily() const;
    };

    static std::shared_ptr<VulkanDevice> Create(std::shared_ptr<VulkanInstance> instance, vk::SurfaceKHR surface,
            const std::vector<const char *> &requiredDeviceExtensions);

    VulkanDevice(std::shared_ptr<VulkanInstance> aInstance, vk::Device aDevice,
            vk::PhysicalDevice aPhysicalDevice, const QueuesProperties &aQueuesProperties);
    ~VulkanDevice();

    vk::Device Get() const { return device; }

    vk::SurfaceCapabilitiesKHR GetSurfaceCapabilities(vk::SurfaceKHR surface) const;

    std::vector<vk::SurfaceFormatKHR> GetSurfaceFormats(vk::SurfaceKHR surface) const;

    std::vector<uint32_t> GetUniqueQueueFamilyIndices() const;

private:
    std::shared_ptr<VulkanInstance> instance;

    vk::Device device;

    vk::PhysicalDevice physicalDevice;

    QueuesProperties queuesProperties;
};

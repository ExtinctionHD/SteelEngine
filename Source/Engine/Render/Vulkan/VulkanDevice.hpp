#pragma once

#include "Engine/Render/Vulkan/VukanInstance.hpp"

class VulkanDevice
{
public:
    static std::shared_ptr<VulkanDevice> Create(std::shared_ptr<VulkanInstance> instance, vk::SurfaceKHR surface,
        const std::vector<const char*> &requiredDeviceExtensions);

    VulkanDevice(std::shared_ptr<VulkanInstance> aInstance, vk::Device aDevice);
    ~VulkanDevice();

    vk::Device Get() const { return device; }

private:
    std::shared_ptr<VulkanInstance> instance;

    vk::Device device;
};

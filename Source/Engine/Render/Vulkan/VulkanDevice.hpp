#pragma once

#include "Engine/Render/Vulkan/Vulkan.hpp"

class VulkanDevice
{
public:
    static std::unique_ptr<VulkanDevice> Create(vk::Instance instance, vk::SurfaceKHR surface,
        const std::vector<const char*> &requiredDeviceExtensions);

    VulkanDevice(vk::Device aDevice);

    vk::Device Get() const { return device.get(); }

private:
    vk::UniqueDevice device;
};

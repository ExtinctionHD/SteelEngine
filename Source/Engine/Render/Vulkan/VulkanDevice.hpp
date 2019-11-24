#pragma once

#include <vulkan/vulkan.hpp>

class VulkanDevice
{
public:
    VulkanDevice() = delete;
    VulkanDevice(const VulkanDevice &) = delete;
    VulkanDevice(vk::Instance instance, const std::vector<const char*> &requiredDeviceExtensions);

    vk::Device Get() { return device.get(); }

private:
    vk::UniqueDevice device;
};

#pragma once

#include <vulkan/vulkan.hpp>

class VulkanDevice
{
public:
    static std::unique_ptr<VulkanDevice> Create(vk::Instance instance, const std::vector<const char*> &requiredDeviceExtensions);

    VulkanDevice(vk::Device aDevice);

    vk::Device Get() const { return device.get(); }

private:
    vk::UniqueDevice device;
};

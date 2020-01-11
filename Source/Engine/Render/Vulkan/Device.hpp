#pragma once

#include "Engine/Render/Vulkan/Instance.hpp"

class Device
{
public:
    struct QueuesProperties
    {
        uint32_t graphicsFamilyIndex;
        uint32_t presentFamilyIndex;

        bool CommonFamily() const;

        std::vector<uint32_t> GetUniqueIndices() const;
    };

    static std::shared_ptr<Device> Create(std::shared_ptr<Instance> instance, vk::SurfaceKHR surface,
            const std::vector<const char *> &requiredDeviceExtensions);

    Device(std::shared_ptr<Instance> aInstance, vk::Device aDevice,
            vk::PhysicalDevice aPhysicalDevice, vk::CommandPool aCommandPool,
            const QueuesProperties &aQueuesProperties);
    ~Device();

    vk::Device Get() const { return device; }

    vk::SurfaceCapabilitiesKHR GetSurfaceCapabilities(vk::SurfaceKHR surface) const;

    std::vector<vk::SurfaceFormatKHR> GetSurfaceFormats(vk::SurfaceKHR surface) const;

    const QueuesProperties &GetQueueProperties() const;

    uint32_t GetMemoryTypeIndex(uint32_t typeBits, vk::MemoryPropertyFlags requiredProperties) const;

private:
    std::shared_ptr<Instance> instance;

    vk::Device device;

    vk::PhysicalDevice physicalDevice;

    vk::CommandPool commandPool;

    QueuesProperties queuesProperties;
};

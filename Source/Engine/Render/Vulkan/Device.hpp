#pragma once

#include <functional>

#include "Engine/Render/Vulkan/Instance.hpp"

using DeviceCommands = std::function<void(vk::CommandBuffer)>;

class Device
{
public:
    struct QueuesProperties
    {
        uint32_t graphicsFamilyIndex;
        uint32_t presentFamilyIndex;

        bool IsOneFamily() const;

        std::vector<uint32_t> GetUniqueIndices() const;
    };

    struct Queues
    {
        vk::Queue graphics;
        vk::Queue present;
    };

    static std::shared_ptr<Device> Create(std::shared_ptr<Instance> instance, vk::SurfaceKHR surface,
            const std::vector<const char *> &requiredDeviceExtensions);

    Device(std::shared_ptr<Instance> aInstance, vk::Device aDevice,
            vk::PhysicalDevice aPhysicalDevice, const QueuesProperties &aQueuesProperties);
    ~Device();

    vk::Device Get() const { return device; }

    const vk::PhysicalDeviceLimits &GetLimits() const { return properties.limits; }

    vk::SurfaceCapabilitiesKHR GetSurfaceCapabilities(vk::SurfaceKHR surface) const;

    std::vector<vk::SurfaceFormatKHR> GetSurfaceFormats(vk::SurfaceKHR surface) const;

    const QueuesProperties &GetQueueProperties() const { return queuesProperties; }

    const Queues &GetQueues() const { return queues; }

    uint32_t GetMemoryTypeIndex(uint32_t typeBits, vk::MemoryPropertyFlags requiredProperties) const;

    void ExecuteOneTimeCommands(DeviceCommands commands) const;

    vk::CommandBuffer AllocateCommandBuffer() const;

private:
    std::shared_ptr<Instance> instance;

    vk::Device device;

    vk::PhysicalDevice physicalDevice;

    vk::PhysicalDeviceProperties properties;

    QueuesProperties queuesProperties;

    Queues queues;

    vk::CommandPool oneTimeCommandsCommandPool;
    vk::CommandPool generalCommandPool;
};

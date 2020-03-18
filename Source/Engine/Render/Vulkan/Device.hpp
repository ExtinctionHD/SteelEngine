#pragma once

#include "Engine/Render/Vulkan/Instance.hpp"

struct DeviceFeatures
{
    bool samplerAnisotropy = false;
};

struct QueuesProperties
{
    uint32_t graphicsFamilyIndex;
    uint32_t presentFamilyIndex;

    bool IsSameFamilies() const;

    std::vector<uint32_t> GetUniqueIndices() const;
};

struct Queues
{
    vk::Queue graphics;
    vk::Queue present;
};

using DeviceCommands = std::function<void(vk::CommandBuffer)>;

enum class CommandsType
{
    eOneTime,
    eLongLived
};

struct CommandsSync
{
    std::vector<vk::Semaphore> waitSemaphores;
    std::vector<vk::PipelineStageFlags> waitStages;
    std::vector<vk::Semaphore> signalSemaphores;
    vk::Fence fence;
};

class Device
{
public:
    static std::shared_ptr<Device> Create(std::shared_ptr<Instance> instance, vk::SurfaceKHR surface,
            const std::vector<const char *> &requiredDeviceExtensions, const DeviceFeatures &requiredDeviceFeatures);

    ~Device();

    vk::Device Get() const { return device; }

    vk::PhysicalDevice GetPhysicalDevice() const { return physicalDevice; }

    const vk::PhysicalDeviceLimits &GetLimits() const { return properties.limits; }

    const vk::PhysicalDeviceRayTracingPropertiesNV &GetRayTracingProperties() const { return rayTracingProperties; }

    vk::SurfaceCapabilitiesKHR GetSurfaceCapabilities(vk::SurfaceKHR surface) const;

    std::vector<vk::SurfaceFormatKHR> GetSurfaceFormats(vk::SurfaceKHR surface) const;

    const QueuesProperties &GetQueueProperties() const { return queuesProperties; }

    const Queues &GetQueues() const { return queues; }

    uint32_t GetMemoryTypeIndex(uint32_t typeBits, vk::MemoryPropertyFlags requiredProperties) const;

    void ExecuteOneTimeCommands(DeviceCommands commands) const;

    vk::CommandBuffer AllocateCommandBuffer(CommandsType type) const;

    void WaitIdle() const;

private:
    std::shared_ptr<Instance> instance;

    vk::Device device;

    vk::PhysicalDevice physicalDevice;

    vk::PhysicalDeviceProperties properties;
    vk::PhysicalDeviceRayTracingPropertiesNV rayTracingProperties;

    QueuesProperties queuesProperties;

    Queues queues;

    vk::Fence oneTimeCommandsFence;

    std::unordered_map<CommandsType, vk::CommandPool> commandPools;

    Device(std::shared_ptr<Instance> instance_, vk::Device device_,
            vk::PhysicalDevice physicalDevice_, const QueuesProperties &queuesProperties_);
};

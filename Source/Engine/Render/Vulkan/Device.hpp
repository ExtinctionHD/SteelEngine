#pragma once

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

struct Queues
{
    struct Description
    {
        uint32_t graphicsFamilyIndex;
        uint32_t presentFamilyIndex;
    };

    vk::Queue graphics;
    vk::Queue present;
};

class Device
{
public:
    struct Features
    {
        uint32_t samplerAnisotropy : 1;
        uint32_t accelerationStructure : 1;
        uint32_t rayTracingPipeline : 1;
        uint32_t descriptorIndexing : 1;
        uint32_t bufferDeviceAddress : 1;
        uint32_t scalarBlockLayout : 1;
        uint32_t updateAfterBind : 1;
        uint32_t rayQuery : 1;
    };

    struct RayTracingProperties
    {
        uint32_t shaderGroupHandleSize;
        uint32_t shaderGroupBaseAlignment;
        uint32_t minScratchOffsetAlignment;
    };

    static std::unique_ptr<Device> Create(
            const Features& requiredFeatures, const std::vector<const char*>& requiredExtensions);

    ~Device();

    vk::Device Get() const { return device; }

    vk::PhysicalDevice GetPhysicalDevice() const { return physicalDevice; }

    const vk::PhysicalDeviceLimits& GetLimits() const { return properties.limits; }

    const RayTracingProperties& GetRayTracingProperties() const { return rayTracingProperties; }

    vk::SurfaceCapabilitiesKHR GetSurfaceCapabilities(vk::SurfaceKHR surface) const;

    std::vector<vk::SurfaceFormatKHR> GetSurfaceFormats(vk::SurfaceKHR surface) const;

    const Queues::Description& GetQueuesDescription() const { return queuesDescription; }

    const Queues& GetQueues() const { return queues; }

    uint32_t GetMemoryTypeIndex(uint32_t typeBits, vk::MemoryPropertyFlags requiredProperties) const;

    vk::DeviceAddress GetAddress(vk::Buffer buffer) const;

    vk::DeviceAddress GetAddress(vk::AccelerationStructureKHR accelerationStructure) const;

    void ExecuteOneTimeCommands(DeviceCommands commands) const;

    vk::CommandBuffer AllocateCommandBuffer(CommandBufferType type) const;

    void WaitIdle() const;

private:
    vk::Device device;
    vk::PhysicalDevice physicalDevice;
    vk::PhysicalDeviceProperties properties;

    RayTracingProperties rayTracingProperties;

    Queues::Description queuesDescription;
    Queues queues;

    CommandBufferSync oneTimeCommandsSync;
    std::map<CommandBufferType, vk::CommandPool> commandPools;

    Device(vk::Device device_, vk::PhysicalDevice physicalDevice_, const Queues::Description& queuesDescription_);
};

#pragma once

#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Resources/Buffer.hpp"

class ResourceUpdateSystem
{
public:
    ResourceUpdateSystem(std::shared_ptr<Device> aDevice, vk::DeviceSize aCapacity);
    ~ResourceUpdateSystem();

    DeviceCommands GetBufferUpdateCommands(BufferHandle handle);

    void UpdateBuffer(BufferHandle handle);

    DeviceCommands GetLayoutUpdateCommands(const ImageResource &imageResource,
            vk::ImageLayout oldLayout, vk::ImageLayout newLayout);

    void UpdateLayout(const ImageResource &imageResource,
            vk::ImageLayout oldLayout, vk::ImageLayout newLayout);

    void ExecuteUpdateCommands();

private:
    std::shared_ptr<Device> device;

    std::pair<vk::Buffer, vk::DeviceMemory> stagingBuffer;

    vk::DeviceSize capacity = 0;
    vk::DeviceSize currentOffset = 0;

    std::list<DeviceCommands> updateCommandsList;
};

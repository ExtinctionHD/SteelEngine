#pragma once

#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Resources/Buffer.hpp"
#include "Engine/Render/Vulkan/Resources/Image.hpp"

class ResourceUpdateSystem
{
public:
    ResourceUpdateSystem(std::shared_ptr<Device> aDevice, vk::DeviceSize aCapacity);
    ~ResourceUpdateSystem();

    DeviceCommands GetBufferUpdateCommands(BufferHandle handle);

    DeviceCommands GeImageUpdateCommands(ImageHandle handle);

    DeviceCommands GetLayoutUpdateCommands(vk::Image image, const vk::ImageSubresourceRange &range,
            vk::ImageLayout oldLayout, vk::ImageLayout newLayout);

    void EnqueueBufferUpdate(BufferHandle handle);

    void EnqueueImageUpdate(ImageHandle handle);

    void EnqueueLayoutUpdate(vk::Image image, const vk::ImageSubresourceRange &range,
            vk::ImageLayout oldLayout, vk::ImageLayout newLayout);

    void ExecuteUpdateCommands();

private:
    std::shared_ptr<Device> device;

    std::pair<vk::Buffer, vk::DeviceMemory> stagingBuffer;

    vk::DeviceSize capacity = 0;
    vk::DeviceSize currentOffset = 0;

    std::list<DeviceCommands> updateCommandsList;
};

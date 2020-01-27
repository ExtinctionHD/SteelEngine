#pragma once

#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Resources/Buffer.hpp"

class ResourceUpdateSystem
{
public:
    ResourceUpdateSystem(std::shared_ptr<Device> aDevice, vk::DeviceSize aCapacity);
    ~ResourceUpdateSystem();

    void ReserveMemory(vk::DeviceSize aSize);
    void RefuseMemory(vk::DeviceSize aSize);

    void UpdateBuffer(BufferHandle handle);

    void UpdateImagesLayout(const std::vector<vk::Image> &images,
            const std::vector<vk::ImageSubresourceRange> &subresourceRanges,
            vk::ImageLayout oldLayout, vk::ImageLayout newLayout);

    void ExecuteUpdateCommands();

private:
    std::shared_ptr<Device> device;

    std::pair<vk::Buffer, vk::DeviceMemory> stagingBuffer;

    vk::DeviceSize capacity = 0;
    vk::DeviceSize size = 0;

    vk::DeviceSize currentOffset = 0;

    std::list<DeviceCommands> updateCommandsList;
};

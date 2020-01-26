#pragma once

#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Resources/Buffer.hpp"
#include "Engine/Render/Vulkan/Resources/Image.hpp"

class TransferSystem
{
public:
    TransferSystem(std::shared_ptr<Device> aDevice, vk::DeviceSize aCapacity);
    ~TransferSystem();

    void ReserveMemory(vk::DeviceSize aSize);
    void RefuseMemory(vk::DeviceSize aSize);

    void TransferImage(const Image &imageDescriptor);
    void TransferBuffer(BufferHandle handle);

    void PerformTransfer();

private:
    std::shared_ptr<Device> device;

    std::pair<vk::Buffer, vk::DeviceMemory> stagingBuffer;

    vk::DeviceSize capacity = 0;
    vk::DeviceSize size = 0;

    vk::DeviceSize currentOffset = 0;

    std::list<DeviceCommands> transferCommandsList;
};

#pragma once

#include "Engine/Render/Vulkan/Device.hpp"

class ImageDescriptor;
class BufferDescriptor;

class TransferSystem
{
public:
    TransferSystem(std::shared_ptr<Device> aDevice, vk::DeviceSize aCapacity);
    ~TransferSystem();

    void Reserve(vk::DeviceSize aSize);
    void Refuse(vk::DeviceSize aSize);

    void TransferImage(const ImageDescriptor &imageDescriptor);
    void TransferBuffer(const BufferDescriptor &bufferDescriptor);

    void PerformTransfer();

private:
    std::shared_ptr<Device> device;

    std::pair<vk::Buffer, vk::DeviceMemory> stagingBuffer;

    vk::DeviceSize capacity = 0;
    vk::DeviceSize size = 0;

    vk::DeviceSize currentOffset = 0;

    std::list<DeviceCommands> transferCommandsList;
};

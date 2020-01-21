#pragma once

#include "Engine/Render/Vulkan/Device.hpp"

class ImageDescriptor;
class BufferDescriptor;

class TransferSystem
{
public:
    static std::shared_ptr<TransferSystem> Create(std::shared_ptr<Device> device, uint32_t capacity = 0);

    TransferSystem(std::shared_ptr<Device> aDevice, uint32_t aCapacity);
    ~TransferSystem();

    void Reserve(uint32_t aSize);
    void Refuse(uint32_t aSize);

    void TransferImage(const ImageDescriptor &imageDescriptor);
    void TransferBuffer(const BufferDescriptor &bufferDescriptor);

    void PerformTransfer();

private:
    std::shared_ptr<Device> device;

    std::pair<vk::Buffer, vk::DeviceMemory> stagingBuffer;

    uint32_t capacity = 0;
    uint32_t size = 0;

    uint32_t currentOffset = 0;

    std::list<DeviceCommands> updateCommandsList;
};

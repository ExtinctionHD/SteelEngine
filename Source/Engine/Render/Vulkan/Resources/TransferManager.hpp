#pragma once

#include "Engine/Render/Vulkan/Device.hpp"

class ImageData;
class BufferData;

class TransferManager
{
public:
    static std::shared_ptr<TransferManager> Create(std::shared_ptr<Device> device, uint32_t capacity = 0);

    TransferManager(std::shared_ptr<Device> aDevice, uint32_t aCapacity);
    ~TransferManager();

    void Reserve(uint32_t aSize);
    void Refuse(uint32_t aSize);

    void RecordImageTransfer(const ImageData &imageData);
    void RecordBufferTransfer(const BufferData &bufferData);

    void TransferResources();

private:
    std::shared_ptr<Device> device;

    std::pair<vk::Buffer, vk::DeviceMemory> stagingBuffer;

    uint32_t capacity = 0;
    uint32_t size = 0;

    uint32_t currentOffset = 0;

    std::list<DeviceCommands> updateCommandsList;
};

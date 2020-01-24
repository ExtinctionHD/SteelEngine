#pragma once

#include <list>

#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Resources/BufferDescriptor.hpp"
#include "Engine/Render/Vulkan/Resources/TransferSystem.hpp"

class BufferManager
{
public:
    BufferManager(std::shared_ptr<Device> aDevice, std::shared_ptr<TransferSystem> aTransferSystem);
    ~BufferManager();

    BufferDescriptor CreateBuffer(const BufferProperties &properties);

    template <class T>
    BufferDescriptor CreateBuffer(const BufferProperties &properties, const std::vector<T> &initialData);

    void ForceUpdate(const BufferDescriptor &aBufferDescriptor);

    void UpdateMarkedBuffers();

    BufferDescriptor Destroy(const BufferDescriptor &aBufferDescriptor);

private:
    std::shared_ptr<Device> device;
    std::shared_ptr<TransferSystem> transferSystem;

    std::list<BufferDescriptor> buffers;
};

template <class T>
BufferDescriptor BufferManager::CreateBuffer(const BufferProperties &properties, const std::vector<T> &initialData)
{
    BufferDescriptor bufferDescriptor = CreateBuffer(properties);
    auto [data, count] = bufferDescriptor.AccessData<T>();

    Assert(initialData.size() <= count);

    std::copy(initialData.begin(), initialData.end(), data);

    auto it = std::find(buffers.begin(), buffers.end(), bufferDescriptor);
    it->MarkForUpdate();

    return bufferDescriptor;
}

#pragma once

#include <list>

#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Resources/Buffer.hpp"
#include "Engine/Render/Vulkan/Resources/TransferSystem.hpp"

class BufferManager
{
public:
    BufferManager(std::shared_ptr<Device> aDevice, std::shared_ptr<TransferSystem> aTransferSystem);
    ~BufferManager();

    BufferHandle CreateBuffer(const BufferDescription &description);

    template <class T>
    BufferHandle CreateBuffer(const BufferDescription &description, const std::vector<T> &initialData);

    void UpdateMarkedBuffers();

    void Destroy(BufferHandle handle);

private:
    std::shared_ptr<Device> device;
    std::shared_ptr<TransferSystem> transferSystem;

    ResourceStorage<Buffer> buffers;
};

template <class T>
BufferHandle BufferManager::CreateBuffer(const BufferDescription &description, const std::vector<T> &initialData)
{
    const BufferHandle handle = CreateBuffer(description);
    auto [data, count] = handle->AccessData<T>();

    Assert(initialData.size() <= count);

    std::copy(initialData.begin(), initialData.end(), data);

    handle->MarkForUpdate();

    return handle;
}

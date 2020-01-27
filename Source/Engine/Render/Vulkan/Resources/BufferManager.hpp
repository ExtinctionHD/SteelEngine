#pragma once

#include <list>

#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Resources/Buffer.hpp"
#include "Engine/Render/Vulkan/Resources/ResourceUpdateSystem.hpp"

class BufferManager
{
public:
    BufferManager(std::shared_ptr<Device> aDevice, std::shared_ptr<ResourceUpdateSystem> aUpdateSystem);
    ~BufferManager();

    BufferHandle CreateBuffer(const BufferDescription &description);

    template <class T>
    BufferHandle CreateBuffer(const BufferDescription &description, const std::vector<T> &initialData);

    void UpdateMarkedBuffers();

    void Destroy(BufferHandle handle);

private:
    std::shared_ptr<Device> device;
    std::shared_ptr<ResourceUpdateSystem> updateSystem;

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

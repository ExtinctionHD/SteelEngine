#pragma once

#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Resources/Buffer.hpp"

struct Mesh;

class BlasGenerator
{
public:
    BlasGenerator(std::shared_ptr<Device> aDevice, std::shared_ptr<BufferManager> aBufferManager);
    ~BlasGenerator();

    vk::AccelerationStructureNV GenerateBlas(const Mesh& mesh);

    void DestroyBlas(vk::AccelerationStructureNV blas);

private:
    struct BlasStorageEntry
    {
        vk::AccelerationStructureNV accelerationStructure;
        vk::DeviceMemory memory;
        BufferHandle scratchBuffer;
    };

    std::shared_ptr<Device> device;
    std::shared_ptr<BufferManager> bufferManager;

    std::list<BlasStorageEntry> blasStorage;
};

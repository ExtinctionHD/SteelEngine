#pragma once

#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Resources/Buffer.hpp"

struct Mesh;

struct GeometryInstance
{
    vk::AccelerationStructureNV blas;
    glm::mat4 transform;
};

class AccelerationStructureManager
{
public:
    AccelerationStructureManager(std::shared_ptr<Device> aDevice, std::shared_ptr<BufferManager> aBufferManager);
    ~AccelerationStructureManager();

    vk::AccelerationStructureNV GenerateBlas(const Mesh &mesh);

    vk::AccelerationStructureNV GenerateTlas(const std::vector<GeometryInstance> &instances);

    void DestroyAccelerationStructure(vk::AccelerationStructureNV accelerationStructure);

private:
    struct AccelerationStructure
    {
        vk::AccelerationStructureNV object;
        vk::DeviceMemory memory;
        BufferHandle scratchBuffer;
    };

    std::shared_ptr<Device> device;
    std::shared_ptr<BufferManager> bufferManager;

    std::list<AccelerationStructure> accelerationStructures;
    std::map<vk::AccelerationStructureNV, BufferHandle> tlasInstanceBuffers;

    AccelerationStructure CreateAccelerationStructure(const vk::AccelerationStructureInfoNV &info) const;
};

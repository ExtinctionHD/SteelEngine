#pragma once

#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/Vulkan/Resources/Buffer.hpp"
#include "Engine/Render/Vulkan/Resources/MemoryManager.hpp"

class RenderObject;

struct GeometryInstance
{
    vk::AccelerationStructureNV blas;
    glm::mat4 transform;
};

class AccelerationStructureManager
{
public:
    AccelerationStructureManager(std::shared_ptr<Device> device_, std::shared_ptr<MemoryManager> memoryManager_,
            std::shared_ptr<BufferManager> bufferManager_);
    ~AccelerationStructureManager();

    vk::AccelerationStructureNV GenerateBlas(const RenderObject &renderObject);

    vk::AccelerationStructureNV GenerateTlas(const std::vector<GeometryInstance> &instances);

    void DestroyAccelerationStructure(vk::AccelerationStructureNV accelerationStructure);

private:
    std::shared_ptr<Device> device;
    std::shared_ptr<MemoryManager> memoryManager;
    std::shared_ptr<BufferManager> bufferManager;

    std::map<vk::AccelerationStructureNV, BufferHandle> accelerationStructures;
    std::map<vk::AccelerationStructureNV, BufferHandle> tlasInstanceBuffers;
};

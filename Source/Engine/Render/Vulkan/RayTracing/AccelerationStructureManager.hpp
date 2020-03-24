#pragma once

#include "Engine/Render/Vulkan/Resources/Buffer.hpp"

class RenderObject;

struct GeometryInstance
{
    vk::AccelerationStructureNV blas;
    glm::mat4 transform;
};

class AccelerationStructureManager
{
public:
    AccelerationStructureManager() = default;
    ~AccelerationStructureManager();

    vk::AccelerationStructureNV GenerateBlas(const RenderObject &renderObject);

    vk::AccelerationStructureNV GenerateTlas(const std::vector<GeometryInstance> &instances);

    void DestroyAccelerationStructure(vk::AccelerationStructureNV accelerationStructure);

private:
    std::map<vk::AccelerationStructureNV, BufferHandle> accelerationStructures;
    std::map<vk::AccelerationStructureNV, BufferHandle> tlasInstanceBuffers;
};

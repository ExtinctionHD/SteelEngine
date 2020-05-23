#pragma once

#include "Engine/Render/Vulkan/RayTracing/AccelerationStructureHelpers.hpp"

class AccelerationStructureManager
{
public:
    vk::AccelerationStructureNV GenerateBlas(const GeometryVertices &vertices, const GeometryIndices &indices);

    vk::AccelerationStructureNV GenerateTlas(const std::vector<GeometryInstance> &instances);

    void DestroyAccelerationStructure(vk::AccelerationStructureNV accelerationStructure);

private:
    std::map<vk::AccelerationStructureNV, vk::Buffer> accelerationStructures;
    std::map<vk::AccelerationStructureNV, vk::Buffer> tlasInstanceBuffers;
};

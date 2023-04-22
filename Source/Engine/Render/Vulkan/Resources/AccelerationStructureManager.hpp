#pragma once

#include "Engine/Render/Vulkan/Resources/AccelerationStructureHelpers.hpp"

class AccelerationStructureManager
{
public:
    vk::AccelerationStructureKHR GenerateUnitBBoxBlas();

    vk::AccelerationStructureKHR GenerateBlas(const BlasGeometryData& geometryData);

    vk::AccelerationStructureKHR GenerateTlas(const std::vector<TlasInstanceData>& instances);

    void DestroyAccelerationStructure(vk::AccelerationStructureKHR accelerationStructure);

private:
    std::map<vk::AccelerationStructureKHR, vk::Buffer> accelerationStructures;
};

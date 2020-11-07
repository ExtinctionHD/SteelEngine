#pragma once

#include "Engine/Render/Vulkan/RayTracing/AccelerationStructureHelpers.hpp"

namespace RT
{
    class AccelerationStructureManager
    {
    public:
        vk::AccelerationStructureKHR GenerateBlas(const GeometryVertexData& vertexData, const GeometryIndexData& indexData);

        vk::AccelerationStructureKHR GenerateTlas(const std::vector<GeometryInstanceData>& instances);

        void DestroyAccelerationStructure(vk::AccelerationStructureKHR accelerationStructure);

    private:
        std::vector<vk::AccelerationStructureKHR> accelerationStructures;
        std::map<vk::AccelerationStructureKHR, vk::Buffer> tlasInstanceBuffers;
    };
}
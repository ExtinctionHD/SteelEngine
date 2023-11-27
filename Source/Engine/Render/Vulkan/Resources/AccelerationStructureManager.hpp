#pragma once

#include "Utils/DataHelpers.hpp"

struct BlasGeometryData
{
    vk::IndexType indexType;
    uint32_t indexCount;
    ByteView indices;

    vk::Format vertexFormat;
    uint32_t vertexStride;
    uint32_t vertexCount;
    ByteView vertices;
};

using TlasInstances = std::vector<vk::AccelerationStructureInstanceKHR>;

class AccelerationStructureManager
{
public:
    vk::AccelerationStructureKHR GenerateBlas(const BlasGeometryData& geometryData);

    vk::AccelerationStructureKHR CreateTlas(const TlasInstances& instances);

    void BuildTlas(vk::CommandBuffer commandBuffer,
            vk::AccelerationStructureKHR tlas, const TlasInstances& instances);

    void DestroyAccelerationStructure(vk::AccelerationStructureKHR accelerationStructure);

private:
    struct AccelerationStructureBuffers
    {
        vk::Buffer sourceBuffer;
        vk::Buffer scratchBuffer;
        vk::Buffer storageBuffer;
    };

    std::map<vk::AccelerationStructureKHR, AccelerationStructureBuffers> accelerationStructures;
};

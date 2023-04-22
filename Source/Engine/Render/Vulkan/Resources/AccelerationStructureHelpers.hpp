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

struct TlasInstanceData
{
    vk::AccelerationStructureKHR blas;
    glm::mat4 transform;
    uint32_t customIndex;
    uint32_t mask;
    uint32_t sbtRecordOffset;
    vk::GeometryInstanceFlagsKHR flags;
};

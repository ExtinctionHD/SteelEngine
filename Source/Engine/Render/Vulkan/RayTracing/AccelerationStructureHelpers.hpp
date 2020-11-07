#pragma once

namespace RT
{
    struct GeometryVertexData
    {
        vk::Buffer buffer;
        vk::Format format;
        uint32_t count;
        uint32_t stride;
    };

    struct GeometryIndexData
    {
        vk::Buffer buffer;
        vk::IndexType type;
        uint32_t count;
    };

    struct GeometryInstanceData
    {
        vk::AccelerationStructureKHR blas;
        glm::mat4 transform;
        uint32_t customIndex;
        uint32_t mask;
        uint32_t sbtRecordOffset;
        vk::GeometryInstanceFlagsKHR flags;
    };
}

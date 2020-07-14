#pragma once

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
    uint32_t customIndex : 24;
    uint32_t mask : 8;
    uint32_t hitGroupIndex : 24;
    uint32_t flags : 8;
};

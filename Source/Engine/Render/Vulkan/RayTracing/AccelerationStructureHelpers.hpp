#pragma once

struct GeometryVertices
{
    vk::Buffer buffer;
    vk::Format format;
    uint32_t count;
    uint32_t stride;
};

struct GeometryIndices
{
    vk::Buffer buffer;
    vk::IndexType type;
    uint32_t count;
};

struct GeometryInstance
{
    vk::AccelerationStructureNV blas;
    glm::mat4 transform;
    uint32_t customIndex : 24;
    uint32_t mask : 8;
    uint32_t hitGroupIndex : 24;
    uint32_t flags : 8;
};

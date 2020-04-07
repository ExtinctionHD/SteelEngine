#pragma once

class RenderObject;

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
};

class AccelerationStructureManager
{
public:
    AccelerationStructureManager() = default;
    ~AccelerationStructureManager();

    vk::AccelerationStructureNV GenerateBlas(const GeometryVertices &vertices, const GeometryIndices &indices);

    vk::AccelerationStructureNV GenerateTlas(const std::vector<GeometryInstance> &instances);

    void DestroyAccelerationStructure(vk::AccelerationStructureNV accelerationStructure);

private:
    std::map<vk::AccelerationStructureNV, vk::Buffer> accelerationStructures;
    std::map<vk::AccelerationStructureNV, vk::Buffer> tlasInstanceBuffers;
};

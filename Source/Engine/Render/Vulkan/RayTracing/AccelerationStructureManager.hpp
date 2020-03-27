#pragma once

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
    std::map<vk::AccelerationStructureNV, vk::Buffer> accelerationStructures;
    std::map<vk::AccelerationStructureNV, vk::Buffer> tlasInstanceBuffers;
};

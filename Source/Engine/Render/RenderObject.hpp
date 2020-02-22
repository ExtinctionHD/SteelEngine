#pragma once

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/Buffer.hpp"

struct Mesh
{
    uint32_t vertexCount;
    VertexFormat vertexFormat;
    BufferHandle vertexBuffer;

    uint32_t indexCount;
    vk::IndexType indexType;
    BufferHandle indexBuffer;
};

struct RenderObject
{
    Mesh mesh;
    vk::AccelerationStructureNV blas;
    std::vector<glm::mat4> transforms;
};

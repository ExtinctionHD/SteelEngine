#pragma once

struct Primitive
{
    struct Vertex
    {
        static const std::vector<vk::Format> kFormat;

        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 tangent;
        glm::vec2 texCoord;
    };

    vk::IndexType indexType = vk::IndexType::eUint16;

    uint32_t indexCount = 0;
    vk::Buffer indexBuffer;

    uint32_t vertexCount = 0;
    vk::Buffer vertexBuffer;

    vk::AccelerationStructureKHR blas;
};
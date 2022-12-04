#pragma once

#include "Utils/AABBox.hpp"
#include "Utils/DataHelpers.hpp"

// TODO: use separate buffers for vertex attributes
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

    vk::IndexType indexType;
    uint32_t indexCount;

    vk::Buffer indexBuffer;
    vk::Buffer vertexBuffer;

    AABBox bbox;
};

namespace PrimitiveHelpers
{
    void CalculateNormals(vk::IndexType indexType,
            const ByteView& indices, std::vector<Primitive::Vertex>& vertices);

    void CalculateTangents(vk::IndexType indexType,
            const ByteView& indices, std::vector<Primitive::Vertex>& vertices);
}

#include "Engine/Scene/Primitive.hpp"

#include "Utils/Assert.hpp"

const std::vector<vk::Format> Primitive::Vertex::kFormat{
    vk::Format::eR32G32B32Sfloat,
    vk::Format::eR32G32B32Sfloat,
    vk::Format::eR32G32B32Sfloat,
    vk::Format::eR32G32Sfloat,
};

namespace PrimitiveHelpers
{
    template<class T>
    static void CalculateNormals(const DataView<T>& indices,
        std::vector<Primitive::Vertex>& vertices)
    {
        for (auto& vertex : vertices)
        {
            vertex.normal = glm::vec3();
        }

        for (size_t i = 0; i < indices.size; i = i + 3)
        {
            const glm::vec3& position0 = vertices[indices[i]].position;
            const glm::vec3& position1 = vertices[indices[i + 1]].position;
            const glm::vec3& position2 = vertices[indices[i + 2]].position;

            const glm::vec3 edge1 = position1 - position0;
            const glm::vec3 edge2 = position2 - position0;

            const glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

            vertices[indices[i]].normal += normal;
            vertices[indices[i + 1]].normal += normal;
            vertices[indices[i + 2]].normal += normal;
        }

        for (auto& vertex : vertices)
        {
            vertex.normal = glm::normalize(vertex.normal);
        }
    }

    template<class T>
    static void CalculateTangents(const DataView<T>& indices,
        std::vector<Primitive::Vertex>& vertices)
    {
        for (auto& vertex : vertices)
        {
            vertex.tangent = glm::vec3();
        }

        for (size_t i = 0; i < indices.size; i = i + 3)
        {
            const glm::vec3& position0 = vertices[indices[i]].position;
            const glm::vec3& position1 = vertices[indices[i + 1]].position;
            const glm::vec3& position2 = vertices[indices[i + 2]].position;

            const glm::vec3 edge1 = position1 - position0;
            const glm::vec3 edge2 = position2 - position0;

            const glm::vec2& texCoord0 = vertices[indices[i]].texCoord;
            const glm::vec2& texCoord1 = vertices[indices[i + 1]].texCoord;
            const glm::vec2& texCoord2 = vertices[indices[i + 2]].texCoord;

            const glm::vec2 deltaTexCoord1 = texCoord1 - texCoord0;
            const glm::vec2 deltaTexCoord2 = texCoord2 - texCoord0;

            float d = deltaTexCoord1.x * deltaTexCoord2.y - deltaTexCoord1.y * deltaTexCoord2.x;

            if (d == 0.0f)
            {
                d = 1.0f;
            }

            const glm::vec3 tangent = (edge1 * deltaTexCoord2.y - edge2 * deltaTexCoord1.y) / d;

            vertices[indices[i]].tangent += tangent;
            vertices[indices[i + 1]].tangent += tangent;
            vertices[indices[i + 2]].tangent += tangent;
        }

        for (auto& vertex : vertices)
        {
            if (glm::length(vertex.tangent) > 0.0f)
            {
                vertex.tangent = glm::normalize(vertex.tangent);
            }
            else
            {
                vertex.tangent.x = 1.0f;
            }
        }
    }

    void CalculateNormals(vk::IndexType indexType,
        const ByteView& indices, std::vector<Primitive::Vertex>& vertices)
    {
        switch (indexType)
        {
        case vk::IndexType::eUint16:
            CalculateNormals(DataView<uint16_t>(indices), vertices);
            break;
        case vk::IndexType::eUint32:
            CalculateNormals(DataView<uint32_t>(indices), vertices);
            break;
        default:
            Assert(false);
            break;
        }
    }

    void CalculateTangents(vk::IndexType indexType,
        const ByteView& indices, std::vector<Primitive::Vertex>& vertices)
    {
        switch (indexType)
        {
        case vk::IndexType::eUint16:
            CalculateTangents(DataView<uint16_t>(indices), vertices);
            break;
        case vk::IndexType::eUint32:
            CalculateTangents(DataView<uint32_t>(indices), vertices);
            break;
        default:
            Assert(false);
            break;
        }
    }
}

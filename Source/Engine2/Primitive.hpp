#pragma once

#include "Utils/DataHelpers.hpp"

struct DefaultVertex
{
    static const std::vector<vk::Format> kFormat;

    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec2 texCoord;
};

struct RayTracingVertex
{
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec2 texCoord;
};

struct Primitive
{
    struct RayTracing
    {
        vk::Buffer indexBuffer;
        vk::Buffer vertexBuffer;
        vk::AccelerationStructureKHR blas;
    };

    vk::IndexType indexType = vk::IndexType::eUint16;

    uint32_t indexCount = 0;
    uint32_t vertexCount = 0;

    vk::Buffer indexBuffer;
    vk::Buffer vertexBuffer;

    RayTracing rayTracing;
};

namespace PrimitiveHelpers
{
    void CalculateNormals(vk::IndexType indexType, 
            const ByteView& indices, std::vector<DefaultVertex>& vertices);

    void CalculateTangents(vk::IndexType indexType, 
            const ByteView& indices, std::vector<DefaultVertex>& vertices);
}
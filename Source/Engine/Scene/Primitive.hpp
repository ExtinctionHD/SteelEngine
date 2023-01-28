#pragma once

#include "Utils/AABBox.hpp"
#include "Utils/DataHelpers.hpp"

struct VertexInput;

struct Primitive
{
    static const std::vector<VertexInput> kVertexInputs;

    std::vector<uint32_t> indices;
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> tangents;
    std::vector<glm::vec2> texCoords;

    vk::Buffer indexBuffer;
    vk::Buffer positionBuffer;
    vk::Buffer normalBuffer;
    vk::Buffer tangentBuffer;
    vk::Buffer texCoordBuffer;

    AABBox bbox;
    
    uint32_t GetIndexCount() const;

    uint32_t GetVertexCount() const;
};

namespace PrimitiveHelpers
{
    Primitive CreatePrimitive(std::vector<uint32_t> indices, 
            std::vector<glm::vec3> positions, std::vector<glm::vec3> normals, 
            std::vector<glm::vec3> tangents, std::vector<glm::vec2> texCoords);
    
    void CreatePrimitiveBuffers(Primitive& primitive);

    void DestroyPrimitiveBuffers(const Primitive& primitive);

    void DrawPrimitive(vk::CommandBuffer commandBuffer, const Primitive& primitive);
}

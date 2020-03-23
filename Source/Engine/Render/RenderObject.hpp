#pragma once

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/Buffer.hpp"
#include "Engine/Render/Vulkan/Resources/Texture.hpp"

struct Vertex
{
    static const VertexFormat kFormat;

    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec2 texCoord;
};

static_assert(sizeof(Vertex) == (sizeof(glm::vec3) * 3 + sizeof(glm::vec2)));

struct Material
{
    Texture baseColor;
};

class RenderObject
{
public:
    RenderObject(const std::vector<Vertex> &vertices_, const std::vector<uint32_t> &indices_,
            BufferHandle vertexBuffer_, BufferHandle indexBuffer_, const Material &material_);

    VertexFormat GetVertexFormat() const { return Vertex::kFormat; }

    vk::IndexType GetIndexType() const { return indexBuffer ? vk::IndexType::eUint32 : vk::IndexType::eNoneNV; }

    uint32_t GetVertexStride() const { return sizeof(Vertex); }

    uint32_t GetVertexCount() const { return static_cast<uint32_t>(vertices.size()); }

    uint32_t GetIndexCount() const { return static_cast<uint32_t>(indices.size()); }

    vk::Buffer GetVertexBuffer() const { return vertexBuffer->buffer; }

    vk::Buffer GetIndexBuffer() const { return indexBuffer->buffer; }

    const Material &GetMaterial() const { return material; }

private:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    BufferHandle vertexBuffer;
    BufferHandle indexBuffer;

    Material material;
};

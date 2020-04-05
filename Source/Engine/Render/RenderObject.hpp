#pragma once

#include "Engine/Render/Vulkan/VulkanHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/Texture.hpp"

struct Vertex
{
    static const VertexFormat kFormat;

    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec2 texCoord;
};

struct Material
{
    std::string name;

    Texture baseColorTexture;
    Texture metallicRoughnessTexture;
    Texture occlusionTexture;
    Texture normalTexture;

    glm::vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    float occlusionFactor;
    float normalFactor;
};

class RenderObject
{
public:
    RenderObject(const std::vector<Vertex> &vertices_,
            const std::vector<uint32_t> &indices_,
            const Material &material_);

    VertexFormat GetVertexFormat() const { return Vertex::kFormat; }

    vk::IndexType GetIndexType() const { return indexBuffer ? vk::IndexType::eUint32 : vk::IndexType::eNoneNV; }

    constexpr uint32_t GetVertexStride() const { return sizeof(Vertex); }

    constexpr uint32_t GetIndexStride() const { return sizeof(uint32_t); }

    uint32_t GetVertexCount() const { return static_cast<uint32_t>(vertices.size()); }

    uint32_t GetIndexCount() const { return static_cast<uint32_t>(indices.size()); }

    const vk::Buffer &GetVertexBuffer() const { return vertexBuffer; }

    const vk::Buffer &GetIndexBuffer() const { return indexBuffer; }

    const Material &GetMaterial() const { return material; }

private:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    Material material;

    vk::Buffer vertexBuffer;
    vk::Buffer indexBuffer;
};

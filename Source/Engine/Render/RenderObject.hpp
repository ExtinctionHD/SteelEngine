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
    Texture surfaceTexture;
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
    static constexpr vk::Format KVertexFormat = vk::Format::eR32G32B32Sfloat;
    static constexpr vk::IndexType kIndexType = vk::IndexType::eUint32;

    static constexpr uint32_t kVertexStride = sizeof(Vertex);
    static constexpr uint32_t kIndexStride = sizeof(uint32_t);

    RenderObject(const std::vector<Vertex> &vertices_,
            const std::vector<uint32_t> &indices_,
            const Material &material_);

    uint32_t GetVertexCount() const { return static_cast<uint32_t>(vertices.size()); }

    uint32_t GetIndexCount() const { return static_cast<uint32_t>(indices.size()); }

    const std::vector<Vertex> &GetVertices() const { return vertices; }

    const std::vector<uint32_t> &GetIndices() const { return indices; }

    const Material &GetMaterial() const { return material; }

private:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    Material material;
};

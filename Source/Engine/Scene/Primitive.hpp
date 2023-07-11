#pragma once

#include "Utils/AABBox.hpp"
#include "Utils/DataHelpers.hpp"

struct VertexInput;

class Primitive
{
public:
    static constexpr vk::IndexType kIndexType = vk::IndexType::eUint32;

    static const std::vector<VertexInput> kVertexInputs;

    Primitive(std::vector<uint32_t> indices_,
            std::vector<glm::vec3> positions_,
            std::vector<glm::vec3> normals_ = {},
            std::vector<glm::vec3> tangents_ = {},
            std::vector<glm::vec2> texCoords_ = {});

    Primitive(const Primitive& other) noexcept;
    Primitive(Primitive&& other) noexcept;

    ~Primitive();

    Primitive& operator=(Primitive other) noexcept;

    uint32_t GetIndexCount() const;

    uint32_t GetVertexCount() const;

    const std::vector<uint32_t>& GetIndices() const { return indices; }
    const std::vector<glm::vec3>& GetPositions() const { return positions; }
    const std::vector<glm::vec3>& GetNormals() const { return normals; }
    const std::vector<glm::vec3>& GetTangents() const { return tangents; }
    const std::vector<glm::vec2>& GetTexCoords() const { return texCoords; }

    const AABBox& GetBBox() const { return bbox; }

    vk::Buffer GetIndexBuffer() const { return indexBuffer; }
    vk::Buffer GetPositionBuffer() const { return positionBuffer; }
    vk::Buffer GetNormalBuffer() const { return normalBuffer; }
    vk::Buffer GetTangentBuffer() const { return tangentBuffer; }
    vk::Buffer GetTexCoordBuffer() const { return texCoordBuffer; }

    vk::AccelerationStructureKHR GetBlas() const { return blas; }

    void Draw(vk::CommandBuffer commandBuffer) const;

private:
    std::vector<uint32_t> indices;
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> tangents;
    std::vector<glm::vec2> texCoords;

    AABBox bbox;

    vk::Buffer indexBuffer;
    vk::Buffer positionBuffer;
    vk::Buffer normalBuffer;
    vk::Buffer tangentBuffer;
    vk::Buffer texCoordBuffer;

    vk::AccelerationStructureKHR blas;

    void CreateBuffers();

    void GenerateBlas();

    void DestroyBuffers() const;

    void DestroyBlas() const;
};

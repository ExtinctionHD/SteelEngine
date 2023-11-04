#include "Engine/Scene/Primitive.hpp"

#include "Engine/Engine.hpp"
#include "Engine/Render/Vulkan/Pipelines/GraphicsPipeline.hpp"
#include "Engine/Render/Vulkan/Resources/BufferHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/ResourceHelpers.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Utils/Assert.hpp"
#include "Utils/Helpers.hpp"

namespace Details
{
    static std::vector<glm::vec3> ComputeNormals(
            const std::vector<uint32_t>& indices,
            const std::vector<glm::vec3>& positions)
    {
        Assert(!positions.empty());

        std::vector<glm::vec3> normals = Repeat(Vector3::kZero, positions.size());

        for (size_t i = 0; i < indices.size(); i = i + 3)
        {
            const glm::vec3& position0 = positions[indices[i]];
            const glm::vec3& position1 = positions[indices[i + 1]];
            const glm::vec3& position2 = positions[indices[i + 2]];

            const glm::vec3 edge1 = position1 - position0;
            const glm::vec3 edge2 = position2 - position0;

            const glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

            normals[indices[i]] += normal;
            normals[indices[i + 1]] += normal;
            normals[indices[i + 2]] += normal;
        }

        for (auto& normal : normals)
        {
            normal = glm::normalize(normal);
        }

        return normals;
    }

    static std::vector<glm::vec3> ComputeTangents(
            const std::vector<uint32_t>& indices,
            const std::vector<glm::vec3>& positions,
            const std::vector<glm::vec2>& texCoords)
    {
        Assert(!positions.empty());
        Assert(!texCoords.empty());

        std::vector<glm::vec3> tangents = Repeat(Vector3::kZero, positions.size());

        for (size_t i = 0; i < indices.size(); i = i + 3)
        {
            const glm::vec3& position0 = positions[indices[i]];
            const glm::vec3& position1 = positions[indices[i + 1]];
            const glm::vec3& position2 = positions[indices[i + 2]];

            const glm::vec3 edge1 = position1 - position0;
            const glm::vec3 edge2 = position2 - position0;

            const glm::vec2& texCoord0 = texCoords[indices[i]];
            const glm::vec2& texCoord1 = texCoords[indices[i + 1]];
            const glm::vec2& texCoord2 = texCoords[indices[i + 2]];

            const glm::vec2 deltaTexCoord1 = texCoord1 - texCoord0;
            const glm::vec2 deltaTexCoord2 = texCoord2 - texCoord0;

            float d = deltaTexCoord1.x * deltaTexCoord2.y - deltaTexCoord1.y * deltaTexCoord2.x;

            if (d == 0.0f)
            {
                d = 1.0f;
            }

            const glm::vec3 tangent = (edge1 * deltaTexCoord2.y - edge2 * deltaTexCoord1.y) / d;

            tangents[indices[i]] += tangent;
            tangents[indices[i + 1]] += tangent;
            tangents[indices[i + 2]] += tangent;
        }

        for (auto& tangent : tangents)
        {
            if (glm::length(tangent) > 0.0f)
            {
                tangent = glm::normalize(tangent);
            }
            else
            {
                tangent.x = 1.0f;
            }
        }

        return tangents;
    }
}

const std::vector<VertexInput> Primitive::kVertexInputs{
    VertexInput{ { vk::Format::eR32G32B32Sfloat }, 0, vk::VertexInputRate::eVertex },
    VertexInput{ { vk::Format::eR32G32B32Sfloat }, 0, vk::VertexInputRate::eVertex },
    VertexInput{ { vk::Format::eR32G32B32Sfloat }, 0, vk::VertexInputRate::eVertex },
    VertexInput{ { vk::Format::eR32G32Sfloat }, 0, vk::VertexInputRate::eVertex }
};

Primitive::Primitive(std::vector<uint32_t> indices_,
        std::vector<glm::vec3> positions_, std::vector<glm::vec3> normals_,
        std::vector<glm::vec3> tangents_, std::vector<glm::vec2> texCoords_)
    : indices(std::move(indices_))
    , positions(std::move(positions_))
    , normals(std::move(normals_))
    , tangents(std::move(tangents_))
    , texCoords(std::move(texCoords_))
{
    if (normals.empty())
    {
        normals = Details::ComputeNormals(indices, positions);
    }
    if (texCoords.empty())
    {
        texCoords = Repeat(Vector2::kZero, GetVertexCount());
    }
    if (tangents.empty())
    {
        tangents = Details::ComputeTangents(indices, positions, texCoords);
    }

    for (const auto& position : positions)
    {
        bbox.Add(position);
    }

    CreateBuffers();

    if (Config::engine.rayTracingEnabled)
    {
        GenerateBlas();
    }
}

Primitive::Primitive(const Primitive& other) noexcept
{
    Assert(false);

    DestroyBuffers();
    DestroyBlas();

    indices = other.indices;
    positions = other.positions;
    normals = other.normals;
    tangents = other.tangents;
    texCoords = other.texCoords;

    bbox = other.bbox;

    indexBuffer = other.indexBuffer;
    positionBuffer = other.positionBuffer;
    normalBuffer = other.normalBuffer;
    tangentBuffer = other.tangentBuffer;
    texCoordBuffer = other.texCoordBuffer;

    blas = other.blas;
}

Primitive::Primitive(Primitive&& other) noexcept
{
    std::swap(indices, other.indices);
    std::swap(positions, other.positions);
    std::swap(normals, other.normals);
    std::swap(tangents, other.tangents);
    std::swap(texCoords, other.texCoords);

    std::swap(bbox, other.bbox);

    std::swap(indexBuffer, other.indexBuffer);
    std::swap(positionBuffer, other.positionBuffer);
    std::swap(normalBuffer, other.normalBuffer);
    std::swap(tangentBuffer, other.tangentBuffer);
    std::swap(texCoordBuffer, other.texCoordBuffer);

    std::swap(blas, other.blas);
}

Primitive::~Primitive()
{
    DestroyBuffers();
    DestroyBlas();
}

Primitive& Primitive::operator=(Primitive other) noexcept
{
    if (this != &other)
    {
        std::swap(indices, other.indices);
        std::swap(positions, other.positions);
        std::swap(normals, other.normals);
        std::swap(tangents, other.tangents);
        std::swap(texCoords, other.texCoords);

        std::swap(bbox, other.bbox);

        std::swap(indexBuffer, other.indexBuffer);
        std::swap(positionBuffer, other.positionBuffer);
        std::swap(normalBuffer, other.normalBuffer);
        std::swap(tangentBuffer, other.tangentBuffer);
        std::swap(texCoordBuffer, other.texCoordBuffer);

        std::swap(blas, other.blas);
    }

    return *this;
}

uint32_t Primitive::GetIndexCount() const
{
    return static_cast<uint32_t>(indices.size());
}

uint32_t Primitive::GetVertexCount() const
{
    return static_cast<uint32_t>(positions.size());
}

void Primitive::CreateBuffers()
{
    constexpr vk::BufferUsageFlags indexUsage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eStorageBuffer;

    constexpr vk::BufferUsageFlags vertexUsage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer;

    Assert(!indices.empty());
    indexBuffer = BufferHelpers::CreateBufferWithData(indexUsage, GetByteView(indices));

    Assert(!positions.empty());
    positionBuffer = BufferHelpers::CreateBufferWithData(vertexUsage, GetByteView(positions));

    Assert(!normals.empty());
    normalBuffer = BufferHelpers::CreateBufferWithData(vertexUsage, GetByteView(normals));

    Assert(!tangents.empty());
    tangentBuffer = BufferHelpers::CreateBufferWithData(vertexUsage, GetByteView(tangents));

    Assert(!texCoords.empty());
    texCoordBuffer = BufferHelpers::CreateBufferWithData(vertexUsage, GetByteView(texCoords));
}

void Primitive::GenerateBlas()
{
    Assert(!indices.empty());
    Assert(!positions.empty());

    BlasGeometryData geometryData;

    geometryData.indexType = Primitive::kIndexType;
    geometryData.indexCount = static_cast<uint32_t>(indices.size());
    geometryData.indices = GetByteView(indices);

    geometryData.vertexFormat = vk::Format::eR32G32B32Sfloat;
    geometryData.vertexStride = sizeof(glm::vec3);
    geometryData.vertexCount = static_cast<uint32_t>(positions.size());
    geometryData.vertices = GetByteView(positions);

    blas = VulkanContext::accelerationStructureManager->GenerateBlas(geometryData);
}

void Primitive::DestroyBuffers() const
{
    if (indexBuffer)
    {
        ResourceHelpers::DestroyResourceDelayed(indexBuffer);
    }
    if (positionBuffer)
    {
        ResourceHelpers::DestroyResourceDelayed(positionBuffer);
    }
    if (normalBuffer)
    {
        ResourceHelpers::DestroyResourceDelayed(normalBuffer);
    }
    if (tangentBuffer)
    {
        ResourceHelpers::DestroyResourceDelayed(tangentBuffer);
    }
    if (texCoordBuffer)
    {
        ResourceHelpers::DestroyResourceDelayed(texCoordBuffer);
    }
}

void Primitive::DestroyBlas() const
{
    if (blas)
    {
        ResourceHelpers::DestroyResourceDelayed(blas);
    }
}

void Primitive::Draw(vk::CommandBuffer commandBuffer) const
{
    std::vector<vk::Buffer> vertexBuffers;

    if (positionBuffer)
    {
        vertexBuffers.push_back(positionBuffer);
    }
    if (normalBuffer)
    {
        vertexBuffers.push_back(normalBuffer);
    }
    if (tangentBuffer)
    {
        vertexBuffers.push_back(tangentBuffer);
    }
    if (texCoordBuffer)
    {
        vertexBuffers.push_back(texCoordBuffer);
    }

    if (!vertexBuffers.empty())
    {
        const std::vector<vk::DeviceSize> offsets(vertexBuffers.size(), 0);

        commandBuffer.bindVertexBuffers(0, vertexBuffers, offsets);
    }

    if (indexBuffer)
    {
        commandBuffer.bindIndexBuffer(indexBuffer, 0, Primitive::kIndexType);

        commandBuffer.drawIndexed(GetIndexCount(), 1, 0, 0, 0);
    }
    else
    {
        commandBuffer.draw(GetVertexCount(), 1, 0, 0);
    }
}

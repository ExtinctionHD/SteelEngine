#include "Engine/Scene/Primitive.hpp"

#include "Engine/Render/Vulkan/Resources/BufferHelpers.hpp"
#include "Engine/Render/Vulkan/GraphicsPipeline.hpp"
#include "Engine/Render/Vulkan/VulkanContext.hpp"

#include "Utils/Assert.hpp"
#include "Utils/Helpers.hpp"

const std::vector<VertexInput> Primitive::kVertexInputs{
    VertexInput{ { vk::Format::eR32G32B32Sfloat }, 0, vk::VertexInputRate::eVertex },
    VertexInput{ { vk::Format::eR32G32B32Sfloat }, 0, vk::VertexInputRate::eVertex },
    VertexInput{ { vk::Format::eR32G32B32Sfloat }, 0, vk::VertexInputRate::eVertex },
    VertexInput{ { vk::Format::eR32G32Sfloat }, 0, vk::VertexInputRate::eVertex }
};

uint32_t Primitive::GetIndexCount() const
{
    return static_cast<uint32_t>(indices.size());
}

uint32_t Primitive::GetVertexCount() const
{
    return static_cast<uint32_t>(positions.size());
}

namespace PrimitiveHelpers
{
    static void CalculateNormals(Primitive& primitive)
    {
        Assert(!primitive.positions.empty());

        primitive.normals = Repeat(Vector3::kZero, primitive.positions.size());

        for (size_t i = 0; i < primitive.indices.size(); i = i + 3)
        {
            const glm::vec3& position0 = primitive.positions[primitive.indices[i]];
            const glm::vec3& position1 = primitive.positions[primitive.indices[i + 1]];
            const glm::vec3& position2 = primitive.positions[primitive.indices[i + 2]];

            const glm::vec3 edge1 = position1 - position0;
            const glm::vec3 edge2 = position2 - position0;

            const glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

            primitive.normals[primitive.indices[i]] += normal;
            primitive.normals[primitive.indices[i + 1]] += normal;
            primitive.normals[primitive.indices[i + 2]] += normal;
        }

        for (auto& normal : primitive.normals)
        {
            normal = glm::normalize(normal);
        }
    }

    static void CalculateTangents(Primitive& primitive)
    {
        Assert(!primitive.positions.empty());
        Assert(!primitive.texCoords.empty());

        primitive.tangents = Repeat(Vector3::kZero, primitive.positions.size());

        for (size_t i = 0; i < primitive.indices.size(); i = i + 3)
        {
            const glm::vec3& position0 = primitive.positions[primitive.indices[i]];
            const glm::vec3& position1 = primitive.positions[primitive.indices[i + 1]];
            const glm::vec3& position2 = primitive.positions[primitive.indices[i + 2]];

            const glm::vec3 edge1 = position1 - position0;
            const glm::vec3 edge2 = position2 - position0;

            const glm::vec2& texCoord0 = primitive.texCoords[primitive.indices[i]];
            const glm::vec2& texCoord1 = primitive.texCoords[primitive.indices[i + 1]];
            const glm::vec2& texCoord2 = primitive.texCoords[primitive.indices[i + 2]];

            const glm::vec2 deltaTexCoord1 = texCoord1 - texCoord0;
            const glm::vec2 deltaTexCoord2 = texCoord2 - texCoord0;

            float d = deltaTexCoord1.x * deltaTexCoord2.y - deltaTexCoord1.y * deltaTexCoord2.x;

            if (d == 0.0f)
            {
                d = 1.0f;
            }

            const glm::vec3 tangent = (edge1 * deltaTexCoord2.y - edge2 * deltaTexCoord1.y) / d;

            primitive.tangents[primitive.indices[i]] += tangent;
            primitive.tangents[primitive.indices[i + 1]] += tangent;
            primitive.tangents[primitive.indices[i + 2]] += tangent;
        }

        for (auto& tangent : primitive.tangents)
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
    }

    static void SetNormals(Primitive& primitive, std::vector<glm::vec3> normals)
    {
        if (!normals.empty())
        {
            Assert(normals.size() == primitive.positions.size());
            primitive.normals = std::move(normals);
        }
        else
        {
            CalculateNormals(primitive);
        }
    }

    static void SetTexCoords(Primitive& primitive, std::vector<glm::vec2> texCoords)
    {
        if (!texCoords.empty())
        {
            Assert(texCoords.size() == primitive.positions.size());
            primitive.texCoords = std::move(texCoords);
        }
        else
        {
            primitive.texCoords = Repeat(Vector2::kZero, primitive.positions.size());
        }
    }
    
    static void SetTangents(Primitive& primitive, std::vector<glm::vec3> tangents)
    {
        if (!tangents.empty())
        {
            Assert(tangents.size() == primitive.positions.size());
            primitive.tangents = std::move(tangents);
        }
        else
        {
            CalculateTangents(primitive);
        }
    }

    static std::vector<vk::Buffer> GetVertexBuffers(const Primitive& primitive)
    {
        std::vector<vk::Buffer> vertexBuffers;

        if (primitive.positionBuffer)
        {
            vertexBuffers.push_back(primitive.positionBuffer);
        }
        if (primitive.normalBuffer)
        {
            vertexBuffers.push_back(primitive.normalBuffer);
        }
        if (primitive.tangentBuffer)
        {
            vertexBuffers.push_back(primitive.tangentBuffer);
        }
        if (primitive.texCoordBuffer)
        {
            vertexBuffers.push_back(primitive.texCoordBuffer);
        }

        return vertexBuffers;
    }
    
    Primitive CreatePrimitive(std::vector<uint32_t> indices,
            std::vector<glm::vec3> positions, std::vector<glm::vec3> normals,
            std::vector<glm::vec3> tangents, std::vector<glm::vec2> texCoords)
    {
        Primitive primitive{};

        Assert(!indices.empty());
        primitive.indices = std::move(indices);

        Assert(!positions.empty());
        primitive.positions = std::move(positions);

        SetNormals(primitive, std::move(normals));
        
        SetTexCoords(primitive, std::move(texCoords));
        
        SetTangents(primitive, std::move(tangents));

        CreatePrimitiveBuffers(primitive);
        
        for (const auto& position : primitive.positions)
        {
            primitive.bbox.Add(position);
        }

        return primitive;
    }

    void CreatePrimitiveBuffers(Primitive& primitive)
    {
        constexpr vk::BufferUsageFlags indexUsage
                = vk::BufferUsageFlagBits::eIndexBuffer
                | vk::BufferUsageFlagBits::eStorageBuffer;

        constexpr vk::BufferUsageFlags vertexUsage
                = vk::BufferUsageFlagBits::eVertexBuffer
                | vk::BufferUsageFlagBits::eStorageBuffer;

        Assert(primitive.GetIndexCount() > 0);
        primitive.indexBuffer = BufferHelpers::CreateBufferWithData(indexUsage, ByteView(primitive.indices));

        Assert(primitive.GetVertexCount() > 0);
        primitive.positionBuffer = BufferHelpers::CreateBufferWithData(vertexUsage, ByteView(primitive.positions));

        if (!primitive.normals.empty())
        {
            primitive.normalBuffer = BufferHelpers::CreateBufferWithData(vertexUsage, ByteView(primitive.normals));
        }
        if (!primitive.tangents.empty())
        {
            primitive.tangentBuffer = BufferHelpers::CreateBufferWithData(vertexUsage, ByteView(primitive.tangents));
        }
        if (!primitive.texCoords.empty())
        {
            primitive.texCoordBuffer = BufferHelpers::CreateBufferWithData(vertexUsage, ByteView(primitive.texCoords));
        }
    }

    void DestroyPrimitiveBuffers(const Primitive& primitive)
    {
        if (primitive.indexBuffer)
        {
            VulkanContext::bufferManager->DestroyBuffer(primitive.indexBuffer);
        }
        if (primitive.positionBuffer)
        {
            VulkanContext::bufferManager->DestroyBuffer(primitive.positionBuffer);
        }
        if (primitive.normalBuffer)
        {
            VulkanContext::bufferManager->DestroyBuffer(primitive.normalBuffer);
        }
        if (primitive.tangentBuffer)
        {
            VulkanContext::bufferManager->DestroyBuffer(primitive.tangentBuffer);
        }
        if (primitive.texCoordBuffer)
        {
            VulkanContext::bufferManager->DestroyBuffer(primitive.texCoordBuffer);
        }
    }

    void DrawPrimitive(vk::CommandBuffer commandBuffer, const Primitive& primitive)
    {
        const std::vector<vk::Buffer> vertexBuffers = GetVertexBuffers(primitive);

        if (!vertexBuffers.empty())
        {
            const std::vector<vk::DeviceSize> offsets(vertexBuffers.size(), 0);

            commandBuffer.bindVertexBuffers(0, vertexBuffers, offsets);
        }

        if (primitive.indexBuffer)
        {
            commandBuffer.bindIndexBuffer(primitive.indexBuffer, 0, vk::IndexType::eUint32);

            commandBuffer.drawIndexed(primitive.GetIndexCount(), 1, 0, 0, 0);
        }
        else
        {
            commandBuffer.draw(primitive.GetVertexCount(), 1, 0, 0);
        }
    }
}

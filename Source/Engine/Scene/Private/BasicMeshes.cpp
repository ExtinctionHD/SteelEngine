#include "Engine/Scene/BasicMeshes.hpp"

#include "Utils/Helpers.hpp"

namespace Details
{
    static Primitive CreateUnitCube()
    {
        const std::vector<uint32_t> indices{
            4, 5, 6,
            6, 7, 4,

            1, 0, 3,
            3, 2, 1,

            0, 4, 7,
            7, 3, 0,

            5, 1, 2,
            2, 6, 5,

            3, 7, 6,
            6, 2, 3,

            0, 1, 5,
            5, 4, 0
        };

        const std::vector<glm::vec3> positions{
            glm::vec3(-0.5f, -0.5f, -0.5f),
            glm::vec3(0.5f, -0.5f, -0.5f),
            glm::vec3(0.5f, 0.5f, -0.5f),
            glm::vec3(-0.5f, 0.5f, -0.5f),
            glm::vec3(-0.5f, -0.5f, 0.5f),
            glm::vec3(0.5f, -0.5f, 0.5f),
            glm::vec3(0.5f, 0.5f, 0.5f),
            glm::vec3(-0.5f, 0.5f, 0.5f),
        };

        return Primitive(indices, positions);
    };

    static Primitive GenerateSphere(float radius, uint32_t sectorCount, uint32_t stackCount)
    {
        const uint32_t vertexCount = (stackCount + 1) * (sectorCount + 1);

        std::vector<glm::vec3> vertices;
        vertices.reserve(vertexCount);

        const float sectorStep = Math::kTwoPi / static_cast<float>(sectorCount);
        const float stackStep = Math::kPi / static_cast<float>(stackCount);

        for (uint32_t i = 0; i <= stackCount; ++i)
        {
            const float stackAngle = Math::kHalfPi - static_cast<float>(i) * stackStep;

            glm::vec3 vertex(0.0f, 0.0f, radius * std::sin(stackAngle));

            for (uint32_t j = 0; j <= sectorCount; ++j)
            {
                const float sectorAngle = static_cast<float>(j) * sectorStep;

                vertex.x = radius * std::cos(stackAngle) * std::cos(sectorAngle);
                vertex.y = radius * std::cos(stackAngle) * std::sin(sectorAngle);

                vertices.push_back(vertex);
            }
        }

        std::vector<uint32_t> indices;
        for (uint32_t i = 0; i < stackCount; ++i)
        {
            uint32_t k1 = i * (sectorCount + 1);
            uint32_t k2 = k1 + sectorCount + 1;

            for (uint32_t j = 0; j < sectorCount; ++j, ++k1, ++k2)
            {
                if (i != 0)
                {
                    indices.push_back(k1);
                    indices.push_back(k2);
                    indices.push_back(k1 + 1);
                }

                if (i != (stackCount - 1))
                {
                    indices.push_back(k1 + 1);
                    indices.push_back(k2);
                    indices.push_back(k2 + 1);
                }
            }
        }

        return Primitive(indices, vertices);
    }

    static Primitive GenerateCylinder(float radius, float height, uint32_t sectorCount)
    {
        const uint32_t ringVertexCount = sectorCount + 1;
        const uint32_t vertexCount = ringVertexCount * 2 + 2;

        std::vector<glm::vec3> vertices;
        vertices.reserve(vertexCount);

        const float halfHeight = height * 0.5f;
        const float sectorStep = Math::kTwoPi / static_cast<float>(sectorCount);

        for (uint32_t i = 0; i <= sectorCount; ++i)
        {
            const float angle = static_cast<float>(i) * sectorStep;
            const float x = radius * std::cos(angle);
            const float y = radius * std::sin(angle);

            vertices.emplace_back(x, y, halfHeight);
            vertices.emplace_back(x, y, -halfHeight);
        }

        vertices.emplace_back(0.0f, 0.0f, halfHeight);
        vertices.emplace_back(0.0f, 0.0f, -halfHeight);

        std::vector<uint32_t> indices;

        for (uint32_t i = 0; i < sectorCount; ++i)
        {
            uint32_t top1 = i * 2;
            uint32_t bot1 = top1 + 1;
            uint32_t top2 = ((i + 1) % ringVertexCount) * 2;
            uint32_t bot2 = top2 + 1;

            indices.push_back(top1);
            indices.push_back(bot1);
            indices.push_back(top2);

            indices.push_back(top2);
            indices.push_back(bot1);
            indices.push_back(bot2);
        }

        const uint32_t topCenter = static_cast<uint32_t>(vertices.size() - 2);
        const uint32_t botCenter = static_cast<uint32_t>(vertices.size() - 1);

        for (uint32_t i = 0; i < sectorCount; ++i)
        {
            const uint32_t next = (i + 1) % ringVertexCount;

            indices.push_back(topCenter);
            indices.push_back(next * 2);
            indices.push_back(i * 2);

            indices.push_back(botCenter);
            indices.push_back(i * 2 + 1);
            indices.push_back(next * 2 + 1);
        }

        return Primitive(indices, vertices);
    }
}

Primitive BasicMeshes::cube;
Primitive BasicMeshes::sphere;
Primitive BasicMeshes::cylinder;

void BasicMeshes::Create()
{
    cube = Details::CreateUnitCube();
    sphere = Details::GenerateSphere(0.5f, 64, 32);
    cylinder = Details::GenerateCylinder(0.5f, 1.0f, 64);
}

void BasicMeshes::Destroy()
{
    cube = {};
    sphere = {};
    cylinder = {};
}

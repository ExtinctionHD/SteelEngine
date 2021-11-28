#include <tetgen.h>

#include "Engine/Scene/MeshHelpers.hpp"

#include "Utils/Helpers.hpp"

namespace Details
{
    static constexpr uint32_t kDefaultSectorCount = 64;
    static constexpr uint32_t kDefaultStackCount = 32;
}

Mesh MeshHelpers::GenerateSphere(float radius, uint32_t sectorCount, uint32_t stackCount)
{
    std::vector<glm::vec3> vertices;
    vertices.reserve((stackCount + 1) * (sectorCount + 1));

    const float sectorStep = 2 * Numbers::kPi / sectorCount;
    const float stackStep = Numbers::kPi / stackCount;

    for (uint32_t i = 0; i <= stackCount; ++i)
    {
        const float stackAngle = Numbers::kPi / 2 - i * stackStep;

        glm::vec3 vertex(0.0f, 0.0f, radius * std::sin(stackAngle));

        for (uint32_t j = 0; j <= sectorCount; ++j)
        {
            const float sectorAngle = j * sectorStep;

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

    return Mesh{ vertices, indices };
}

Mesh MeshHelpers::GenerateSphere(float radius)
{
    return GenerateSphere(radius, Details::kDefaultSectorCount, Details::kDefaultStackCount);
}

std::vector<Tetrahedron> MeshHelpers::GenerateTetrahedralMesh(const std::vector<glm::vec3>& vertices)
{
    constexpr size_t tetrahedronVertexCount = GetSize<decltype(Tetrahedron::vertices)>();

    if (vertices.size() < tetrahedronVertexCount)
    {
        return {};
    }

    std::vector<glm::tvec3<double>> points(vertices.size());
    for (size_t i = 0; i < vertices.size(); ++i)
    {
        points[i].x = static_cast<double>(vertices[i].x);
        points[i].y = static_cast<double>(vertices[i].y);
        points[i].z = static_cast<double>(vertices[i].z);
    }

    tetgenbehavior behaviour;

    tetgenio input;
    input.pointlist = reinterpret_cast<double*>(points.data());
    input.numberofpoints = static_cast<int>(vertices.size());

    tetgenio output;

    tetrahedralize(&behaviour, &input, &output);

    input.pointlist = nullptr;

    std::vector<Tetrahedron> mesh(output.numberoftetrahedra);
    for (size_t i = 0; i < mesh.size(); ++i)
    {
        for (size_t j = 0; j < tetrahedronVertexCount; ++j)
        {
            const size_t index = i * tetrahedronVertexCount + j;

            mesh[i].vertices[j] = output.tetrahedronlist[index];
        }
    }

    return mesh;
}

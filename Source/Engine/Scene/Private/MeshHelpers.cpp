#include <tetgen.h>

#include "Engine/Scene/MeshHelpers.hpp"

#include "Utils/Assert.hpp"
#include "Utils/Helpers.hpp"

namespace Details
{
    static constexpr uint32_t kDefaultSectorCount = 64;
    static constexpr uint32_t kDefaultStackCount = 32;

    static constexpr size_t kTetrahedronVertexCount = 4;
    static constexpr size_t kTriangleVertexCount = 3;

    using TetrahedronVertices = std::array<glm::vec3, kTetrahedronVertexCount>;
    using TetrahedronFaceIndices = std::array<size_t, kTriangleVertexCount>;

    glm::mat3x4 CalculateTetrahedronMatrix(const TetrahedronVertices& tetrahedronVertices)
    {
        const glm::vec3& a = tetrahedronVertices[0];
        const glm::vec3& b = tetrahedronVertices[1];
        const glm::vec3& c = tetrahedronVertices[2];
        const glm::vec3& d = tetrahedronVertices[3];

        const glm::mat3 matrix{
            glm::vec3(a.x - d.x, b.x - d.x, c.x - d.x),
            glm::vec3(a.y - d.y, b.y - d.y, c.y - d.y),
            glm::vec3(a.z - d.z, b.z - d.z, c.z - d.z),
        };

        const glm::mat3 result = glm::inverse(matrix);

        Assert(Matrix3::IsValid(result));

        return result;
    }

    TetrahedronFaceIndices GetTetrahedronOppositeFaceIndices(size_t index)
    {
        constexpr size_t tetrahedronVertexCount = GetSize<decltype(Tetrahedron::vertices)>();

        Assert(index < tetrahedronVertexCount);

        TetrahedronFaceIndices oppositeFaceIndices{};

        for (size_t i = 0; i < oppositeFaceIndices.size(); ++i)
        {
            oppositeFaceIndices[i] = (index + i + 1) % tetrahedronVertexCount;
        }

        return oppositeFaceIndices;
    }
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

TetrahedralData MeshHelpers::GenerateTetrahedral(const std::vector<glm::vec3>& vertices)
{
    if (vertices.size() < Details::kTetrahedronVertexCount)
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
    behaviour.neighout = 1;
    behaviour.edgesout = 1;

    tetgenio input;
    input.pointlist = reinterpret_cast<double*>(points.data());
    input.numberofpoints = static_cast<int>(vertices.size());

    tetgenio output;

    tetrahedralize(&behaviour, &input, &output);

    std::vector<Tetrahedron> tetrahedral(output.numberoftetrahedra);

    for (size_t i = 0; i < tetrahedral.size(); ++i)
    {
        Details::TetrahedronVertices tetrahedronVertices{};

        for (size_t j = 0; j < Details::kTetrahedronVertexCount; ++j)
        {
            const size_t index = i * Details::kTetrahedronVertexCount + j;

            tetrahedral[i].vertices[j] = output.tetrahedronlist[index];
            tetrahedral[i].neighbors[j] = output.neighborlist[index];

            tetrahedronVertices[j] = vertices[tetrahedral[i].vertices[j]];
        }

        tetrahedral[i].matrix = Details::CalculateTetrahedronMatrix(tetrahedronVertices);
    }

    input.pointlist = nullptr;

    std::vector<uint32_t> edgesIndices(output.numberofedges * 2);
    for (size_t i = 0; i < edgesIndices.size(); ++i)
    {
        edgesIndices[i] = static_cast<uint32_t>(output.edgelist[i]);
    }

    return TetrahedralData{ tetrahedral, edgesIndices };
}

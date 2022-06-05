#pragma once

#include "Shaders/Common/Common.h"

struct Mesh
{
    std::vector<glm::vec3> vertices;
    std::vector<uint32_t> indices;
};

struct TetrahedralData
{
    std::vector<gpu::Tetrahedron> tetrahedral;
    std::vector<uint32_t> edgesIndices;
};

namespace MeshHelpers
{
    Mesh GenerateSphere(float radius, uint32_t sectorCount, uint32_t stackCount);

    Mesh GenerateSphere(float radius);

    TetrahedralData GenerateTetrahedral(const std::vector<glm::vec3>& vertices);
}

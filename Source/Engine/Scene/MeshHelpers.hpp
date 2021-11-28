#pragma once

#include "Shaders/Hybrid/Hybrid.h"

struct Mesh
{
    std::vector<glm::vec3> vertices;
    std::vector<uint32_t> indices;
};

namespace MeshHelpers
{
    Mesh GenerateSphere(float radius, uint32_t sectorCount, uint32_t stackCount);

    Mesh GenerateSphere(float radius);

    std::vector<Tetrahedron> GenerateTetrahedralMesh(const std::vector<glm::vec3>& vertices);
}

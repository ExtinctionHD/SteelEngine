#pragma once

#include "Shaders/Common/Common.h"


struct TetrahedralData
{
    std::vector<gpu::Tetrahedron> tetrahedral;
    std::vector<uint32_t> edgesIndices;
};

namespace MeshHelpers
{

    TetrahedralData GenerateTetrahedral(const std::vector<glm::vec3>& vertices);
}

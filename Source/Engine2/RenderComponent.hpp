#pragma once
#include "Engine/Filesystem/Filepath.hpp"
#include "Utils/DataHelpers.hpp"
#include "Shaders2/Common/Common.h"

namespace Steel
{
    struct Geometry
    {
        vk::IndexType indexType = vk::IndexType::eUint16;

        ByteView indices;
        DataView<glm::vec3> positions;
        DataView<glm::vec3> normals;
        DataView<glm::vec3> tangents;
        DataView<glm::vec2> texCoords;
    };

    struct Material
    {
        struct Technique
        {
            bool doubleSide = false;
            bool alphaTest = false;
        };

        Technique technique;
        MaterialData data{};
    };

    struct RenderComponent
    {
        uint32_t geometry;
        uint32_t material;
    };
}

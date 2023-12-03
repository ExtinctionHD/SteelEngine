#pragma once

#include "Engine/Render/Vulkan/Shaders/ShaderManager.hpp"

#include "Shaders/Common/Common.h"

#include "Utils/Flags.hpp"

struct Range;

enum class MaterialFlagBits
{
    eAlphaTest,
    eAlphaBlend,
    eDoubleSided,
    eNormalMapping
};

using MaterialFlags = Flags<MaterialFlagBits>;

OVERLOAD_LOGIC_OPERATORS(MaterialFlags, MaterialFlagBits)

struct Material
{
    gpu::Material data;
    MaterialFlags flags;
};

namespace MaterialHelpers
{
    ShaderDefines GetShaderDefines(MaterialFlags flags);

    vk::GeometryInstanceFlagsKHR GetTlasInstanceFlags(MaterialFlags flags);

    void AddTextureOffset(Material& material, int32_t offset);

    void SubtractTextureOffset(Material& material, int32_t offset);

    void SubtractTextureRange(Material& material, const Range& range);
}

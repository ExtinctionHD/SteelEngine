#pragma once

#include "Engine/Render/Vulkan/Shaders/ShaderManager.hpp"

#include "Shaders/Common/Common.h"

#include "Utils/Flags.hpp"

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
    ShaderDefines BuildShaderDefines(MaterialFlags flags);

    vk::GeometryInstanceFlagsKHR GetTlasInstanceFlags(MaterialFlags flags);

    void ApplyTextureOffset(Material& material, int32_t offset);

    void RemoveTextureOffset(Material& material, int32_t offset);
}

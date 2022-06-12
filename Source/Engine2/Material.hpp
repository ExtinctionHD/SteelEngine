#pragma once

#include "Engine/Render/Vulkan/Shaders/ShaderManager.hpp"
#include "Utils/Flags.hpp"

#include "Shaders/Common/Common.h"

enum class MaterialFlagBits
{
    eAlphaTest,
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
}
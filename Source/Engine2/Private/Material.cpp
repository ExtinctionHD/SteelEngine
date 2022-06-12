#include "Engine2/Material.hpp"

ShaderDefines MaterialHelpers::BuildShaderDefines(MaterialFlags flags)
{
    ShaderDefines defines;

    if (flags & MaterialFlagBits::eAlphaTest)
    {
        defines.emplace("ALPHA_TEST", 1);
    }
    if (flags & MaterialFlagBits::eDoubleSided)
    {
        defines.emplace("DOUBLE_SIDED", 1);
    }
    if (flags & MaterialFlagBits::eNormalMapping)
    {
        defines.emplace("NORMAL_MAPPING", 1);
    }

    return defines;
}

#include "Engine/Scene/Material.hpp"

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

vk::GeometryInstanceFlagsKHR MaterialHelpers::GetTlasInstanceFlags(MaterialFlags flags)
{
    vk::GeometryInstanceFlagsKHR instanceFlags;

    if (!(flags & MaterialFlagBits::eAlphaTest))
    {
        instanceFlags |= vk::GeometryInstanceFlagBitsKHR::eForceOpaque;
    }
    if (flags & MaterialFlagBits::eDoubleSided)
    {
        instanceFlags |= vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable;
    }

    return instanceFlags;
}

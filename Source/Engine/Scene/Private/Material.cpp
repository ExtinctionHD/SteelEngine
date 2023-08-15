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

void MaterialHelpers::ApplyTextureOffset(Material& material, int32_t offset)
{
    if (material.data.baseColorTexture >= 0)
    {
        material.data.baseColorTexture += offset;
    }
    if (material.data.roughnessMetallicTexture >= 0)
    {
        material.data.roughnessMetallicTexture += offset;
    }
    if (material.data.normalTexture >= 0)
    {
        material.data.normalTexture += offset;
    }
    if (material.data.occlusionTexture >= 0)
    {
        material.data.occlusionTexture += offset;
    }
    if (material.data.emissionTexture >= 0)
    {
        material.data.emissionTexture += offset;
    }
}

void MaterialHelpers::RemoveTextureOffset(Material& material, int32_t offset)
{
    if (material.data.baseColorTexture >= offset)
    {
        material.data.baseColorTexture -= offset;
    }
    if (material.data.roughnessMetallicTexture >= offset)
    {
        material.data.roughnessMetallicTexture -= offset;
    }
    if (material.data.normalTexture >= offset)
    {
        material.data.normalTexture -= offset;
    }
    if (material.data.occlusionTexture >= offset)
    {
        material.data.occlusionTexture -= offset;
    }
    if (material.data.emissionTexture >= offset)
    {
        material.data.emissionTexture -= offset;
    }
}

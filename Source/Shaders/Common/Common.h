#ifndef COMMON_H
#define COMMON_H

#ifdef __cplusplus
#define mat4 glm::mat4
#define vec4 glm::vec4
namespace ShaderData
{
#endif
    struct MaterialFactors
    {
        vec4 baseColorFactor;
        vec4 emissionFactor;
        float roughnessFactor;
        float metallicFactor;
        float normalScale;
        float occlusionStrength;
    };
#ifdef __cplusplus
}
#undef mat4
#undef vec4
#endif

#endif

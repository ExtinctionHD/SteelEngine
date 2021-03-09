#ifndef HYBRID_H
#define HYBRID_H

#ifdef __cplusplus
#define mat4 glm::mat4
#define vec4 glm::vec4
#endif

struct Material
{
    vec4 baseColorFactor;
    vec4 emissionFactor;
    float roughnessFactor;
    float metallicFactor;
    float normalScale;
    float occlusionStrength;
};

struct CameraST
{
    float S;
    float T;
    float aspectRatio;
    float tanHalfFov;
    mat4 inverseView;
};

#ifdef __cplusplus
#undef mat4
#undef vec4
#endif

#endif

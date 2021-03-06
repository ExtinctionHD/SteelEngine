#ifndef RAY_TRACING_H
#define RAY_TRACING_H

#ifdef __cplusplus
#define mat4 glm::mat4
#define vec4 glm::vec4
#define vec3 glm::vec3
#define vec2 glm::vec2
#endif

struct MaterialRT
{
    int baseColorTexture;
    int roughnessMetallicTexture;
    int normalTexture;
    int emissionTexture;

    vec4 baseColorFactor;
    vec4 emissionFactor;

    float roughnessFactor;
    float metallicFactor;
    float normalScale;
    
    float alphaCutoff;
};

#ifdef __cplusplus
#undef mat4
#undef vec4
#undef vec3
#undef vec2
#endif

#endif

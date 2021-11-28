#ifndef HYBRID_H
#define HYBRID_H

#ifdef __cplusplus
#define mat4 glm::mat4
#define vec4 glm::vec4
#define mat3x4 glm::mat3x4
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

struct Tetrahedron
{
    int vertices[4];
    int neighbors[4];
    mat3x4 matrix;
};

#ifdef __cplusplus
#undef mat4
#undef vec4
#endif

#endif

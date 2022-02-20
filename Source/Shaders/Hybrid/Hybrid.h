#ifndef HYBRID_H
#define HYBRID_H

#ifdef __cplusplus
#define mat4 glm::mat4
#define vec4 glm::vec4
#define mat3x4 glm::mat3x4
#endif

#define VERTEX_COUNT 4
#define COEFFICIENT_COUNT 9

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
    int vertices[VERTEX_COUNT];
    int neighbors[VERTEX_COUNT];
    mat3x4 matrix;
};

#ifdef __cplusplus
#undef mat4
#undef vec4
#undef mat3x4
#endif

#endif

#ifndef COMMON_H
#define COMMON_H

#define TET_VERTEX_COUNT 4
#define COEFFICIENT_COUNT 9

#ifdef __cplusplus
namespace gpu { using namespace glm;
#endif

struct PointLight
{
    vec4 position;
    vec4 color;
};

struct DirectLight
{
    vec4 direction;
    vec4 color;
};

struct MaterialData
{
    vec4 baseColorFactor;
    vec4 emissionFactor;
    int baseColorTexture;
    int roughnessMetallicTexture;
    int normalTexture;
    int occlusionTexture;
    int emissionTexture;
    float roughnessFactor;
    float metallicFactor;
    float normalScale;
    float occlusionStrength;
    float alphaCutoff;
    vec2 padding;
};

struct Tetrahedron
{
    int vertices[TET_VERTEX_COUNT];
    int neighbors[TET_VERTEX_COUNT];
    mat3x4 matrix;
};

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

struct CameraPT
{
    mat4 inverseView;
    mat4 inverseProj;
    float zNear;
    float zFar;
};

#ifdef __cplusplus
static_assert(sizeof(MaterialData) % sizeof(glm::vec4) == 0); }
#endif

#endif

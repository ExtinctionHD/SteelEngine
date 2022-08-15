#ifndef COMMON_H
#define COMMON_H

#define TET_VERTEX_COUNT 4
#define COEFFICIENT_COUNT 9

#ifdef __cplusplus
namespace gpu { using namespace glm;
#endif

struct Light
{
    vec4 location;
    vec4 color;
};

struct Material
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

struct VertexRT
{
    vec4 normal; // .w - texCoord.x
    vec4 tangent; // .w - texCoord.y
};

struct Tetrahedron
{
    int vertices[TET_VERTEX_COUNT];
    int neighbors[TET_VERTEX_COUNT];
    mat3x4 matrix;
};

struct CameraPT
{
    mat4 inverseView;
    mat4 inverseProj;
    float zNear;
    float zFar;
};

#ifdef __cplusplus
}
#endif

#endif

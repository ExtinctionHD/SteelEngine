#ifndef COMMON_H
#define COMMON_H

#define MAX_LIGHT_COUNT 16
#define MAX_MATERIAL_COUNT 256
#define MAX_TEXTURE_COUNT 1024
#define MAX_PRIMITIVE_COUNT 2048

#define TET_VERTEX_COUNT 4
#define SH_COEFFICIENT_COUNT 9

#ifdef __cplusplus
namespace gpu { using namespace glm;
#define CHECK_ALIGNMENT(Struct) static_assert(sizeof(Struct) % sizeof(vec4) == 0)
#endif

#ifdef __cplusplus
    #define START_ENUM(Name) enum class Name {
    #define END_ENUM() }
#else
    #define START_ENUM(Name)  const uint
    #define END_ENUM() 
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
    vec2 _padding;
};

struct Atmosphere
{
    float planetRadius;
    float atmosphereRadius;

    vec3 rayleightScattering;
    float rayleightDensityHeight;

    float mieScattering;
    float mieAbsorption;
    float mieDensityHeight;
    float mieScatteringAsymmetry;

    vec3 ozoneAbsorption;
    float ozoneCenterHeight;
    float ozoneThickness;
};

struct Frame
{
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    mat4 inverseView;
    mat4 inverseProj;
    mat4 inverseProjView;
    vec3 cameraPosition;
    float cameraNearPlaneZ;
    float cameraFarPlaneZ;
    float globalTime;
    Atmosphere atmo;
};

struct Tetrahedron
{
    int vertices[TET_VERTEX_COUNT];
    int neighbors[TET_VERTEX_COUNT];
    mat3x4 matrix;
};

#ifdef __cplusplus

CHECK_ALIGNMENT(Light);
CHECK_ALIGNMENT(Material);
CHECK_ALIGNMENT(Tetrahedron);

}
#endif

#endif

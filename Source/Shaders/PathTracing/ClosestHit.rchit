#version 460
#extension GL_NV_ray_tracing : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#define SHADER_STAGE closest
#pragma shader_stage(closest)

#include "PathTracing/PathTracing.glsl"

layout(set = 4, binding = 0) readonly buffer IndicesData{
    uint indices[];
} indicesData[];

layout(set = 5, binding = 0) readonly buffer PositionsData{
    vec3 positions[];
} positionsData[];

layout(set = 6, binding = 0) readonly buffer NormalsData{
    uint normals[];
} normalsData[];

layout(set = 7, binding = 0) readonly buffer TangentsData{
    uint tangents[];
} tangentsData[];

layout(set = 8, binding = 0) readonly buffer TexCoordsData{
    uint texCoords[];
} texCoordsData[];

layout(location = 0) rayPayloadInNV Payload payload;

hitAttributeNV vec2 hit;

void main()
{
    payload.value = vec3(1 - hit.x - hit.y, hit.x, hit.y);
}

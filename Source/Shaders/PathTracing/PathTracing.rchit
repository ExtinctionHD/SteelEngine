#version 460
#extension GL_NV_ray_tracing : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#define SHADER_STAGE closest
#pragma shader_stage(closest)

#include "PathTracing/PathTracing.h"
#include "PathTracing/PathTracing.glsl"

layout(set = 2, binding = 0) readonly buffer VertexBuffers{
    VertexData vertices[];
} vertexBuffers[];

layout(set = 3, binding = 0) readonly buffer IndexBuffers{
    uint indices[];
} indexBuffers[];

layout(set = 4, binding = 0) uniform sampler2D baseColorTextures[];

layout(location = 0) rayPayloadInNV vec3 outColor;

hitAttributeNV vec2 hit;

void main()
{
    const vec3 barycentrics = vec3(1.0f - hit.x - hit.y, hit.x, hit.y);

    const uint i0 = indexBuffers[nonuniformEXT(gl_InstanceCustomIndexNV)].indices[gl_PrimitiveID * 3];
    const uint i1 = indexBuffers[nonuniformEXT(gl_InstanceCustomIndexNV)].indices[gl_PrimitiveID * 3 + 1];
    const uint i2 = indexBuffers[nonuniformEXT(gl_InstanceCustomIndexNV)].indices[gl_PrimitiveID * 3 + 2];

    VertexData v0 = vertexBuffers[nonuniformEXT(gl_InstanceCustomIndexNV)].vertices[i0];
    VertexData v1 = vertexBuffers[nonuniformEXT(gl_InstanceCustomIndexNV)].vertices[i1];
    VertexData v2 = vertexBuffers[nonuniformEXT(gl_InstanceCustomIndexNV)].vertices[i2];

    const vec2 texCoord = Lerp(v0.texCoord.xy, v1.texCoord.xy, v2.texCoord.xy, barycentrics);
    const vec3 baseColor = texture(baseColorTextures[nonuniformEXT(gl_InstanceCustomIndexNV)], texCoord).rgb;

    outColor = baseColor;
}

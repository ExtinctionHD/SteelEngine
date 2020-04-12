#version 460
#extension GL_NV_ray_tracing : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#define SHADER_STAGE closest
#pragma shader_stage(closest)

#include "PathTracing/PathTracing.h"
#include "PathTracing/PathTracing.glsl"
#include "Common/Common.h"
#include "Common/Common.glsl"
#include "Common/PBR.glsl"

layout(set = 1, binding = 0) uniform accelerationStructureNV tlas;
layout(set = 1, binding = 2) uniform Lighting{
    LightingData lighting;
};

layout(set = 2, binding = 0) readonly buffer VertexBuffers{
    VertexData vertices[];
} vertexBuffers[];

layout(set = 3, binding = 0) readonly buffer IndexBuffers{
    uint indices[];
} indexBuffers[];

layout(set = 4, binding = 0) uniform sampler2D baseColorTextures[];
layout(set = 5, binding = 0) uniform sampler2D surfaceTextures[];
layout(set = 6, binding = 0) uniform sampler2D occlusionTextures[];
layout(set = 7, binding = 0) uniform sampler2D normalTextures[];

layout(location = 0) rayPayloadInNV vec3 outColor;
layout(location = 1) rayPayloadNV float shadow;

hitAttributeNV vec2 hit;

void TraceShadowRay()
{
    const uint flags = gl_RayFlagsOpaqueNV | gl_RayFlagsTerminateOnFirstHitNV | gl_RayFlagsSkipClosestHitShaderNV;
    const vec3 origin = gl_WorldRayOriginNV + gl_WorldRayDirectionNV * gl_HitTNV;

    shadow = 1.0f;
    traceNV(tlas,
            flags,
            0xFF,
            0, 0, 1,
            origin,
            MIN_SHADOW_DISTANCE,
            -lighting.direction.xyz,
            MAX_SHADOW_DISTANCE,
            1);
}

VertexData FetchVertexData(uint offset)
{
    const uint index = indexBuffers[nonuniformEXT(gl_InstanceCustomIndexNV)].indices[gl_PrimitiveID * 3 + offset];
    return vertexBuffers[nonuniformEXT(gl_InstanceCustomIndexNV)].vertices[index];
}

void main()
{    
    TraceShadowRay();

    const vec3 barycentrics = vec3(1.0f - hit.x - hit.y, hit.x, hit.y);

    const VertexData v0 = FetchVertexData(0);
    const VertexData v1 = FetchVertexData(1);
    const VertexData v2 = FetchVertexData(2);

    const vec3 normal = gl_ObjectToWorldNV * Lerp(v0.normal, v1.normal, v2.normal, barycentrics);
    const vec3 tangent = gl_ObjectToWorldNV * Lerp(v0.tangent, v1.tangent, v2.tangent, barycentrics);
    const vec2 texCoord = Lerp(v0.texCoord.xy, v1.texCoord.xy, v2.texCoord.xy, barycentrics);

    const vec3 baseColorSample = texture(baseColorTextures[nonuniformEXT(gl_InstanceCustomIndexNV)], texCoord).rgb;
    const vec2 roughnessMetallicSample = texture(surfaceTextures[nonuniformEXT(gl_InstanceCustomIndexNV)], texCoord).gb;
    const float occlusionSample = texture(occlusionTextures[nonuniformEXT(gl_InstanceCustomIndexNV)], texCoord).r;
    const vec3 normalSample = texture(normalTextures[nonuniformEXT(gl_InstanceCustomIndexNV)], texCoord).rgb * 2.0f - 1.0f;

    const vec3 N = normalize(GetTBN(normal, tangent) * normalSample);
    const vec3 V = normalize(-gl_WorldRayDirectionNV);
    const vec3 L = normalize(-lighting.direction.xyz);

    outColor = CalculatePBR(normal, N, V, L,
            lighting.colorIntensity.rgb,
            lighting.colorIntensity.a,
            ToLinear(baseColorSample),
            roughnessMetallicSample.r,
            roughnessMetallicSample.g,
            occlusionSample, shadow);
}

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
#include "Common/MonteCarlo.glsl"
#include "Common/Random.glsl"

#define RAY_MIN 0.001
#define RAY_MAX 1000
#define DIFFUSE_SAMPLE_COUNT 16
#define SPECULAR_SAMPLE_COUNT 16


layout(set = 1, binding = 0) uniform accelerationStructureNV tlas;
layout(set = 1, binding = 2) uniform Lighting{
    LightingData lighting;
};
layout(set = 1, binding = 3) uniform samplerCube environmentMap;

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

layout(location = 0) rayPayloadInNV Payload payload;
layout(location = 1) rayPayloadNV float shadow;

hitAttributeNV vec2 hit;

VertexData FetchVertexData(uint offset)
{
    const uint index = indexBuffers[nonuniformEXT(gl_InstanceCustomIndexNV)].indices[gl_PrimitiveID * 3 + offset];
    return vertexBuffers[nonuniformEXT(gl_InstanceCustomIndexNV)].vertices[index];
}

void main()
{
    const vec3 barycentrics = vec3(1 - hit.x - hit.y, hit.x, hit.y);

    const VertexData v0 = FetchVertexData(0);
    const VertexData v1 = FetchVertexData(1);
    const VertexData v2 = FetchVertexData(2);

    const vec3 normal = gl_ObjectToWorldNV * Lerp(v0.normal, v1.normal, v2.normal, barycentrics);
    const vec3 tangent = gl_ObjectToWorldNV * Lerp(v0.tangent, v1.tangent, v2.tangent, barycentrics);
    const vec2 texCoord = Lerp(v0.texCoord.xy, v1.texCoord.xy, v2.texCoord.xy, barycentrics);
    const mat3 TBN = GetTBN(normal, tangent);

    const vec3 baseColorSample = texture(baseColorTextures[nonuniformEXT(gl_InstanceCustomIndexNV)], texCoord).rgb;
    const vec2 roughnessMetallicSample = texture(surfaceTextures[nonuniformEXT(gl_InstanceCustomIndexNV)], texCoord).gb;
    const float occlusionSample = texture(occlusionTextures[nonuniformEXT(gl_InstanceCustomIndexNV)], texCoord).r;
    const vec3 normalSample = texture(normalTextures[nonuniformEXT(gl_InstanceCustomIndexNV)], texCoord).rgb * 2 - 1;

    Surface surface;
    surface.baseColor = ToLinear(baseColorSample);
    surface.roughness = roughnessMetallicSample.x;
    surface.metallic = roughnessMetallicSample.y;
    surface.N = normalize(TangentToWorld(normalSample, TBN));
    surface.F0 = mix(DIELECTRIC_F0, surface.baseColor, surface.metallic);
	surface.a  = surface.roughness * surface.roughness;
	surface.a2 = surface.a * surface.a;

    const vec3 p = gl_WorldRayOriginNV + gl_HitTNV * gl_WorldRayDirectionNV;
    const vec3 wo = normalize(-gl_WorldRayDirectionNV);
    const vec3 wi = normalize(-lighting.direction.xyz);
    
    payload.color = CalculatePBR(surface, occlusionSample, 0, wo, wi, normalize(wo + wi),
            lighting.colorIntensity.rgb, lighting.colorIntensity.a);

    /*
    if (payload.depth < MAX_DEPTH)
    {
        payload.depth++;
        
        vec3 diffuse = vec3(0.0);
        for (uint i = 0; i < DIFFUSE_SAMPLE_COUNT; ++i)
        {
            vec2 E = Hammersley(i, DIFFUSE_SAMPLE_COUNT, NextUVec2(payload.seed));
            vec3 wm = TangentToWorld(CosineSampleHemisphere(E).xyz, TBN);

            traceNV(tlas, 
                    gl_RayFlagsOpaqueNV,
                    0xFF,
                    0, 0, 0,
                    p,
                    RAY_MIN,
                    wm,
                    RAY_MAX,
                    0);

            vec3 kD = 1 - F_Schlick(surface.F0, wo, wm);
            diffuse += kD * surface.baseColor * payload.color * INVERSE_PI;
        }
        diffuse /= DIFFUSE_SAMPLE_COUNT;

        vec3 specular = vec3(0.0);
        for (uint i = 0; i < SPECULAR_SAMPLE_COUNT; ++i)
        {
            vec2 E = Hammersley(i, SPECULAR_SAMPLE_COUNT, NextUVec2(payload.seed));
            vec3 wm = TangentToWorld(ImportanceSampleGGX(E, surface.a2).xyz, TBN);
            vec3 wi = reflect(-wo, wm);

            traceNV(tlas, 
                    gl_RayFlagsOpaqueNV,
                    0xFF,
                    0, 0, 0,
                    p,
                    RAY_MIN,
                    wi,
                    RAY_MAX,
                    0);

            vec3 kS = F_Schlick(surface.F0, wo, wm);
            specular += kS * payload.color;
        }
        specular /= SPECULAR_SAMPLE_COUNT;

        payload.color = diffuse + specular;
    }
    else
    {
        payload.color = texture(environmentMap, gl_WorldRayDirectionNV).rgb;
    }
    */
}

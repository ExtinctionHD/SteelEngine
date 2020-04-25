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
#define ENVIRONMENT_INTENSITY 2

layout(set = 1, binding = 0) uniform accelerationStructureNV tlas;
layout(set = 1, binding = 2) uniform Lighting{
    LightingData lighting;
};
layout(set = 1, binding = 3) uniform samplerCube envMap;

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

layout(location = 0) rayPayloadInNV Payload raygen;
layout(location = 1) rayPayloadNV float envHit;

hitAttributeNV vec2 hit;

VertexData FetchVertexData(uint offset)
{
    const uint index = indexBuffers[nonuniformEXT(gl_InstanceCustomIndexNV)].indices[gl_PrimitiveID * 3 + offset];
    return vertexBuffers[nonuniformEXT(gl_InstanceCustomIndexNV)].vertices[index];
}

vec3 TraceEnvironment(vec3 p, vec3 wi)
{
    envHit = 0.0f;
    traceNV(tlas,
            gl_RayFlagsOpaqueNV | gl_RayFlagsTerminateOnFirstHitNV | gl_RayFlagsSkipClosestHitShaderNV,
            0xFF,
            0, 0, 1,
            p,
            RAY_MIN,
            wi,
            RAY_MAX,
            1);

    return ENVIRONMENT_INTENSITY * texture(envMap, wi).rgb * envHit;
}

vec3 SampleEnvironmentEmitting(Surface surface, vec3 p, out vec3 wi, out float pdf)
{
    wi = CosineSampleHemisphere(NextVec2(raygen.seed));
    pdf = CosinePdfHemisphere(CosThetaTangent(wi));

    const vec3 wiWorld = TangentToWorld(wi.xyz, surface.TBN);
    return TraceEnvironment(p, wi);
}

vec3 SampleEnvironmentScattering(Surface surface, vec3 p, vec3 wo, out vec3 wi, out float pdf)
{
    const vec3 bsdf = SampleBSDF(surface, wo, wi, pdf, raygen.seed);
    if (IsBlack(bsdf) || pdf < EPSILON)
    {
        return vec3(0);
    }

    const vec3 wiWorld = TangentToWorld(wi.xyz, surface.TBN);
    return bsdf * TraceEnvironment(p, wiWorld);
}

vec3 CalculateEnvironmentLighting(Surface surface, vec3 p, vec3 wo)
{
    vec3 L = vec3(0);

    vec3 emittingWi; float emittingPdf;
    const vec3 emitting = SampleEnvironmentEmitting(surface, p, emittingWi, emittingPdf);

    if (Emits(emitting))
    {   
        const vec3 wi = emittingWi;
        const vec3 wh = normalize(wo + wi);
        const vec3 bsdf = EvaluateBSDF(surface, wo, emittingWi, wh);
        const float weight = PowerHeuristic(emittingPdf, PdfBSDF(surface, wo, wi, wh));

        L += (emitting * bsdf * CosThetaTangent(wi) * weight) / emittingPdf;
    }

    vec3 scatteringWi; float scatteringPdf;
    const vec3 scattering = SampleEnvironmentScattering(surface, p, wo, scatteringWi, scatteringPdf);
    if (Emits(scattering))
    {
        const vec3 wi = scatteringWi;
        const float weight = PowerHeuristic(scatteringPdf, emittingPdf);
        L += (scattering * CosThetaTangent(wi) * weight) / scatteringPdf;
    }

    return raygen.T * L;
}

void main()
{
    const vec3 barycentrics = vec3(1 - hit.x - hit.y, hit.x, hit.y);

    const VertexData v0 = FetchVertexData(0);
    const VertexData v1 = FetchVertexData(1);
    const VertexData v2 = FetchVertexData(2);

    const vec3 normal = gl_ObjectToWorldNV * BaryLerp(v0.normal, v1.normal, v2.normal, barycentrics);
    const vec3 tangent = gl_ObjectToWorldNV * BaryLerp(v0.tangent, v1.tangent, v2.tangent, barycentrics);
    const vec2 texCoord = BaryLerp(v0.texCoord.xy, v1.texCoord.xy, v2.texCoord.xy, barycentrics);

    const vec3 baseColorSample = texture(baseColorTextures[nonuniformEXT(gl_InstanceCustomIndexNV)], texCoord).rgb;
    const vec2 roughnessMetallicSample = texture(surfaceTextures[nonuniformEXT(gl_InstanceCustomIndexNV)], texCoord).gb;
    //const float occlusionSample = texture(occlusionTextures[nonuniformEXT(gl_InstanceCustomIndexNV)], texCoord).r;
    //const vec3 normalSample = texture(normalTextures[nonuniformEXT(gl_InstanceCustomIndexNV)], texCoord).rgb * 2 - 1;

    Surface surface;
    surface.TBN = GetTBN(normal, tangent);
    surface.baseColor = ToLinear(baseColorSample);
    surface.roughness = RemapRoughness(roughnessMetallicSample.x);
    surface.metallic = roughnessMetallicSample.y;
    //surface.N = normalize(TangentToWorld(normalSample, surface.TBN));
    surface.F0 = mix(DIELECTRIC_F0, surface.baseColor, surface.metallic);
	surface.a  = surface.roughness * surface.roughness;
	surface.a2 = surface.a * surface.a;
    surface.sw = GetSpecularWeight(surface.baseColor, surface.F0, surface.metallic);
    
    const vec3 p = gl_WorldRayOriginNV + gl_HitTNV * gl_WorldRayDirectionNV;
    const vec3 wo = normalize(WorldToTangent(-gl_WorldRayDirectionNV, surface.TBN));

    raygen.L += CalculateEnvironmentLighting(surface, p, wo);
}

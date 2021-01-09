#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_ray_query : require
#extension GL_GOOGLE_include_directive : require

#define SHADER_STAGE fragment
#pragma shader_stage(fragment)

#include "Common/Common.h"
#include "Common/Common.glsl"
#include "Common/PBR.glsl"
#include "Forward/Forward.h"

#define RAY_MIN_T 0.001
#define RAY_MAX_T 1000.0
#define RAY_OFFSET 0.005

layout(set = 0, binding = 1) uniform cameraBuffer{ vec3 cameraPosition; };

layout(set = 1, binding = 0) uniform samplerCube irradianceMap;
layout(set = 1, binding = 1) uniform samplerCube reflectionMap;
layout(set = 1, binding = 2) uniform sampler2D specularBRDF;
layout(set = 1, binding = 3) uniform lightingBuffer{ DirectLight directLight; };

layout(set = 2, binding = 0) uniform accelerationStructureEXT tlas;

layout(set = 3, binding = 0) uniform sampler2D baseColorTexture;
layout(set = 3, binding = 1) uniform sampler2D roughnessMetallicTexture;
layout(set = 3, binding = 2) uniform sampler2D normalTexture;
layout(set = 3, binding = 3) uniform sampler2D occlusionTexture;
layout(set = 3, binding = 4) uniform sampler2D emissionTexture;

layout(set = 3, binding = 5) uniform materialBuffer{ Material material; };

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

float TraceShadowRay(vec3 origin, vec3 direction)
{
    rayQueryEXT rayQuery;

    rayQueryInitializeEXT(rayQuery, tlas, 
            gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT, 0xFF,
            origin, RAY_MIN_T, direction, RAY_MAX_T);

    while(rayQueryProceedEXT(rayQuery)) {}  

    if (rayQueryGetIntersectionTypeEXT(rayQuery, true) != gl_RayQueryCommittedIntersectionNoneEXT)
    {
        return 1.0;
    }

    return 0.0;
}

void main() 
{
    const vec3 baseColor = ToLinear(texture(baseColorTexture, inTexCoord).rgb) * material.baseColorFactor.rgb;

    const vec2 roughnessMetallic = texture(roughnessMetallicTexture, inTexCoord).gb;
    const float roughness = roughnessMetallic.r * material.roughnessFactor;
    const float metallic = roughnessMetallic.g * material.metallicFactor;

    vec3 normalSample = texture(normalTexture, inTexCoord).xyz * 2.0 - 1.0;
    normalSample = normalize(normalSample * vec3(material.normalScale, material.normalScale, 1.0));

    const float occlusion = texture(occlusionTexture, inTexCoord).r * material.occlusionStrength;
    const vec3 emission = ToLinear(texture(emissionTexture, inTexCoord).rgb) * material.emissionFactor.rgb;

    const float a = roughness * roughness;
    const float a2 = a * a;

    const vec3 F0 = mix(DIELECTRIC_F0, baseColor, metallic);

    const vec3 N = normalize(TangentToWorld(normalSample, GetTBN(inNormal, inTangent)));
    const vec3 L = normalize(-directLight.direction.xyz);
    const vec3 V = normalize(normalize(cameraPosition - inPosition));
    const vec3 H = normalize(L + V);
    
    const float NoV = CosThetaWorld(N, V);
    const float NoL = CosThetaWorld(N, L);
    const float NoH = CosThetaWorld(N, H);
    const float VoH = max(dot(V, H), 0.0);

    vec3 directLighting;
    {
        const float D = D_GGX(a2, NoH);
        const vec3 F = F_Schlick(F0, VoH);
        const float Vis = Vis_Schlick(a, NoV, NoL);
        
        const vec3 kD = mix(vec3(1.0) - F, vec3(0.0), metallic);
        
        const vec3 diffuse = kD * Diffuse_Lambert(baseColor);
        const vec3 specular = D * F * Vis;
        
        const float shadow = TraceShadowRay(inPosition + N * RAY_OFFSET, L);
        const vec3 lighting = NoL * directLight.color.rgb * (1.0 - shadow);

        directLighting = (diffuse + specular) * lighting;
    }

    vec3 ambientLighting;
    {
        const vec3 irradiance = texture(irradianceMap, N).rgb;

        const vec3 kS = F_Schlick(F0, NoV);
        const vec3 kD = mix(vec3(1.0) - kS, vec3(0.0), metallic);

        const vec3 R = -reflect(V, N);
        const float lod = roughness * (textureQueryLevels(reflectionMap) - 1);
        const vec3 reflection = textureLod(reflectionMap, R, lod).rgb;

        const vec2 scaleOffset = texture(specularBRDF, vec2(NoV, roughness)).xy;
        
        const vec3 diffuse = kD * irradiance * baseColor;
        const vec3 specular = (F0 * scaleOffset.x + scaleOffset.y) * reflection;

        ambientLighting = (diffuse + specular) * occlusion;
    }

    outColor = vec4(ToneMapping(ambientLighting + directLighting + emission), 1.0);
}
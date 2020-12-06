#version 460
#extension GL_GOOGLE_include_directive : require

#define SHADER_STAGE fragment
#pragma shader_stage(fragment)

#include "Common/Common.glsl"
#include "Common/PBR.glsl"
#include "Forward/Forward.h"

#define LIGHT_INTENSITY 4.0
#define LIGHT_DIRECTION vec3(-0.51, -0.76, 0.41)

layout(set = 0, binding = 1) uniform cameraBuffer{ vec3 cameraPosition; };

layout(set = 1, binding = 0) uniform sampler2D baseColorTexture;
layout(set = 1, binding = 1) uniform sampler2D roughnessMetallicTexture;
layout(set = 1, binding = 2) uniform sampler2D normalTexture;
layout(set = 1, binding = 3) uniform sampler2D occlusionTexture;
layout(set = 1, binding = 4) uniform sampler2D emissionTexture;

layout(set = 1, binding = 5) uniform materialBuffer{ Material material; };

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

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
    const vec3 L = -normalize(LIGHT_DIRECTION);
    const vec3 V = normalize(normalize(cameraPosition - inPosition));
    const vec3 H = normalize(L + V);
    
    const float NoV = max(dot(N, V), 0.0);
    const float NoL = max(dot(N, L), 0.0);
    const float NoH = max(dot(N, H), 0.0);
    const float VoH = max(dot(V, H), 0.0);

    const float D = D_GGX(a2, NoH);
    const vec3 F = F_Schlick(F0, VoH);
    const float G = G_Smith(roughness, NoV, NoL);

    const vec3 kD = mix(vec3(1.0) - F, vec3(0.0), metallic);
    
    const vec3 diffuse = kD * Diffuse_Lambert(baseColor);
    const vec3 specular = D * F * G / (4 * NoV * NoL + EPSILON);

    const vec3 resultColor = (diffuse + specular) * NoL * LIGHT_INTENSITY + emission;

    outColor = vec4(ToneMapping(resultColor), 1.0);
}
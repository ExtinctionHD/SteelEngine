#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#define SHADER_STAGE fragment
#pragma shader_stage(fragment)

#define ALPHA_TEST 0
#define DOUBLE_SIDED 0

#define POINT_LIGHT_COUNT 4

#include "Common/Common.h"
#include "Common/Common.glsl"
#include "Hybrid/Hybrid.h"

layout(push_constant) uniform PushConstants{
    layout(offset = 64) vec3 cameraPosition;
};

layout(set = 1, binding = 0) uniform sampler2D baseColorTexture;
layout(set = 1, binding = 1) uniform sampler2D roughnessMetallicTexture;
layout(set = 1, binding = 2) uniform sampler2D normalTexture;
layout(set = 1, binding = 3) uniform sampler2D occlusionTexture;
layout(set = 1, binding = 4) uniform sampler2D emissionTexture;

layout(set = 1, binding = 5) uniform materialBuffer{ 
    Material material;
#if ALPHA_TEST
    float alphaCutoff;
    float padding[3];
#endif
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec4 outNormal;
layout(location = 1) out vec4 outBaseColor;
layout(location = 2) out vec4 outEmission;
layout(location = 3) out vec4 outMisc;

void main() 
{
#if ALPHA_TEST
    const vec4 baseColor = texture(baseColorTexture, inTexCoord) * material.baseColorFactor;

    if (baseColor.a < alphaCutoff)
    {
        discard;
    }

#else
    const vec4 baseColor = texture(baseColorTexture, inTexCoord) * material.baseColorFactor;
#endif

    vec3 normalSample = texture(normalTexture, inTexCoord).xyz * 2.0 - 1.0;
    normalSample = normalize(normalSample * vec3(material.normalScale, material.normalScale, 1.0));

#if DOUBLE_SIDED
    const vec3 V = normalize(cameraPosition - inPosition);
    const vec3 polygonN = FaceForward(inNormal, V);
#else
    const vec3 polygonN = inNormal;
#endif

    const vec3 normal = normalize(TangentToWorld(normalSample, GetTBN(polygonN, inTangent)));
    const vec3 emission = texture(emissionTexture, inTexCoord).rgb * material.emissionFactor.rgb;
    const vec2 roughnessMetallic = texture(roughnessMetallicTexture, inTexCoord).gb;

    const float roughness = roughnessMetallic.r * material.roughnessFactor;
    const float metallic = roughnessMetallic.g * material.metallicFactor;
    const float occlusion = texture(occlusionTexture, inTexCoord).r * material.occlusionStrength;

    outNormal.xyz = normal.xyz;
    outBaseColor.rgb = baseColor.rgb;
    outEmission.rgb = emission.rgb;
    outMisc.rgb = vec3(roughness, metallic, occlusion);
}
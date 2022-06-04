#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#define SHADER_STAGE fragment
#pragma shader_stage(fragment)

#include "Common/Common.h"
#include "Common/Common.glsl"
#include "Hybrid/Hybrid.h"

#define ALPHA_TEST 0
#define DOUBLE_SIDED 0
#define NORMAL_MAPPING 0

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
layout(location = 2) in vec2 inTexCoord;
#if NORMAL_MAPPING
layout(location = 3) in vec3 inTangent;
#endif

layout(location = 0) out vec4 gBuffer0;
layout(location = 1) out vec4 gBuffer1;
layout(location = 2) out vec4 gBuffer2;
layout(location = 3) out vec4 gBuffer3;

void main() 
{
#if ALPHA_TEST
    const vec4 baseColor = texture(baseColorTexture, inTexCoord) * material.baseColorFactor;

    if (baseColor.a < alphaCutoff)
    {
        discard;
    }

    const vec3 albedo = baseColor.rgb;
#else
    const vec3 albedo = texture(baseColorTexture, inTexCoord).rgb * material.baseColorFactor.rgb;
#endif

#if DOUBLE_SIDED
    const vec3 V = normalize(cameraPosition - inPosition);
    const vec3 polygonN = FaceForward(inNormal, V);
#else
    const vec3 polygonN = inNormal;
#endif

#if NORMAL_MAPPING
    vec3 normalSample = texture(normalTexture, inTexCoord).xyz * 2.0 - 1.0;
    normalSample = normalize(normalSample * vec3(material.normalScale, material.normalScale, 1.0));
    const vec3 normal = normalize(TangentToWorld(normalSample, GetTBN(polygonN, inTangent)));
#else
    const vec3 normal = polygonN;
#endif

    const vec3 emission = texture(emissionTexture, inTexCoord).rgb * material.emissionFactor.rgb;

    const vec2 roughnessMetallic = texture(roughnessMetallicTexture, inTexCoord).gb * vec2(material.roughnessFactor, material.metallicFactor);

    const float occlusion = texture(occlusionTexture, inTexCoord).r * material.occlusionStrength;

    gBuffer0.rgb = normal.xyz * 0.5 + 0.5;
    gBuffer1.rgb = emission;
    gBuffer2.rgba = vec4(albedo.rgb, occlusion);
    gBuffer3.rg = roughnessMetallic;
}
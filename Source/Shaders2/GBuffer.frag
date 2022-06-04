#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#define SHADER_STAGE fragment
#pragma shader_stage(fragment)

#include "Common/Common.h"
#include "Common/Common.glsl"

#define ALPHA_TEST 0
#define DOUBLE_SIDED 0

layout(constant_id = 1) const uint MATERIAL_COUNT = 256;

layout(set = 0, binding = 0) uniform sampler2D textures[];
layout(set = 0, binding = 0) uniform materialUBO{ MaterialData materials[MATERIAL_COUNT]; };
layout(set = 0, binding = 0) uniform dynamicUBO{ DynamicData dynamic; };

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 gBuffer0; // normals
layout(location = 1) out vec3 gBuffer1; // emission
layout(location = 2) out vec4 gBuffer2; // albedo + occlusion
layout(location = 3) out vec2 gBuffer3; // roughness + metallic

void main() 
{
    gBuffer0 = vec3(1.0, 0.0, 0.0);
}
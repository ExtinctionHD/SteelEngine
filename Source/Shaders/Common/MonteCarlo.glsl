#ifndef MONTE_CARLO_GLSL
#define MONTE_CARLO_GLSL

#ifndef SHADER_STAGE
#define SHADER_STAGE vertex
#pragma shader_stage(vertex)
void main() {}
#endif

#include "Common/Constants.glsl"

uint ReverseBits32(uint bits) 
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return bits;
}

vec2 Hammersley(uint i, uint N, uvec2 random)
{
	const float E1 = fract(float(i) / N + float(random.x & 0xFFFFu) / (1u << 16u));
	const float E2 = float(ReverseBits32(i) ^ random.y) * 2.3283064365386963e-10;
	return vec2(E1, E2);
}

vec4 ImportanceSampleGGX(vec2 E, float a2)
{
	float Phi = 2 * PI * E.x;
	float CosTheta = sqrt((1 - E.y) / (1.0f + (a2 - 1) * E.y));
	float SinTheta = sqrt(1 - CosTheta * CosTheta);

	vec3 H;
	H.x = SinTheta * cos(Phi);
	H.y = SinTheta * sin(Phi);
	H.z = CosTheta;
	
	float d = (CosTheta * a2 - CosTheta) * CosTheta + 1;
	float D = a2 / (PI * d * d);
	float PDF = D * CosTheta;

	return vec4(H, PDF);
}

#endif
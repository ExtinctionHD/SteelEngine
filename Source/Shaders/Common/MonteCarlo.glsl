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
    bits = (bits << 16) | (bits >> 16);
    bits = ((bits & 0x55555555) << 1) | ((bits & 0xAAAAAAAA) >> 1);
    bits = ((bits & 0x33333333) << 2) | ((bits & 0xCCCCCCCC) >> 2);
    bits = ((bits & 0x0F0F0F0F) << 4) | ((bits & 0xF0F0F0F0) >> 4);
    bits = ((bits & 0x00FF00FF) << 8) | ((bits & 0xFF00FF00) >> 8);
    return bits;
}

vec2 Hammersley(uint i, uint N, uvec2 random)
{
	float E1 = fract(float(i) / N + float(random.x & 0xFFFF) / (1 << 16));
	float E2 = float(ReverseBits32(i) ^ random.y) * 2.3283064365386963e-10;
	return vec2(E1, E2);
}

vec4 ImportanceSampleGGX(vec2 E, float a2)
{
	float Phi = 2 * PI * E.x;
	float CosTheta = sqrt((1 - E.y) / (1 + (a2 - 1) * E.y));
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
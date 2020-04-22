#ifndef RANDOM_GLSL
#define RANDOM_GLSL

#ifndef SHADER_STAGE
#define SHADER_STAGE vertex
#pragma shader_stage(vertex)
void main() {}
#endif

uint rotl(uint x, uint k) 
{
	return (x << k) | (x >> (32 - k));
}

// xoroshiro64 RNG
uint Rand(inout uvec2 seed) 
{
	const uint result = rotl(seed.x * 0x9E3779BB, 5) * 5;

	seed.y ^= seed.x;
	seed.x = rotl(seed.x, 26) ^ seed.y ^ (seed.y << 9);
	seed.y = rotl(seed.y, 13);

	return result;
}

// Thomas Wang 32-bit hash
uint GetHash(uint seed)
{
    seed = (seed ^ 61) ^ (seed >> 16);
    seed = seed + (seed << 3);
    seed = seed ^ (seed >> 4);
    seed = seed * 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed;
}

uvec2 GetSeed(uvec2 id, uint frameIndex)
{
    const uint s0 = (id.x << 16) | id.y;
    const uint s1 = frameIndex;

    uvec2 seed = uvec2(GetHash(s0), GetHash(s1));
    Rand(seed);

    return seed;
}

float RandFloat(inout uvec2 seed)
{
    uint u = 0x3f800000 | (Rand(seed) >> 9);
    return uintBitsToFloat(u) - 1.0f;
}

vec2 RandVec2(inout uvec2 seed)
{
    return vec2(RandFloat(seed), RandFloat(seed));
}

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
	float x = fract(float(i) / N + float(random.x & 0xffff) / (1 << 16));
	float y = float(ReverseBits32(i) ^ random.y) * 2.3283064365386963e-10;
	return vec2(x, y);
}

#endif
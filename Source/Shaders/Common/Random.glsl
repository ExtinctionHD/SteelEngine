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
    uint u = 0x3F800000 | (Rand(seed) >> 9);
    return uintBitsToFloat(u) - 1;
}

vec2 RandVec2(inout uvec2 seed)
{
    return vec2(RandFloat(seed), RandFloat(seed));
}

#endif
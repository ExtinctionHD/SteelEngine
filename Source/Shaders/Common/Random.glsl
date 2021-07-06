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

float NextFloat(inout uvec2 seed)
{
    uint u = 0x3F800000 | (Rand(seed) >> 9);
    return uintBitsToFloat(u) - 1.0;
}

vec2 NextVec2(inout uvec2 seed)
{
    return vec2(NextFloat(seed), NextFloat(seed));
}

vec3 NextVec3(inout uvec2 seed)
{
    return vec3(NextFloat(seed), NextFloat(seed), NextFloat(seed));
}

uvec2 NextUVec2(inout uvec2 seed)
{
    return uvec2(Rand(seed), Rand(seed));
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

#endif

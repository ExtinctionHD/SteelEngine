#ifndef RAY_TRACING_GLSL
#define RAY_TRACING_GLSL

#ifndef SHADER_STAGE
#define SHADER_STAGE vertex
#pragma shader_stage(vertex)
void main() {}
#endif

#define RAY_MIN_T 0.001
#define RAY_MAX_T 1000.0

struct Ray
{
    vec3 origin;
    vec3 direction;
    float TMin;
    float TMax;
};

struct Sphere
{
    vec3 center;
    float radius;
};

float SphereIntersection(Sphere sphere, Ray ray)
{
    const vec3 L = ray.origin - sphere.center;

    const float a = dot(ray.direction, ray.direction);
    const float b = 2.0 * dot(L, ray.direction);
    const float c = dot(L, L) - sphere.radius * sphere.radius;

    const float D = b * b - 4.0 * a * c;

    if(D < 0.0)
    {
        return -1.0;
    }

    return (-b - sqrt(D)) / (2.0 * a);
}

bool IsMiss(float t)
{
    return t < 0.0;
}

bool IsHit(float t)
{
    return t >= 0.0;
}

#endif
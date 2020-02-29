#version 460
#extension GL_NV_ray_tracing : require

struct Payload
{
    vec3 color;
};

layout(location = 0) rayPayloadInNV Payload payload;

hitAttributeNV vec2 attr;

void main()
{
    const vec3 barycentrics = vec3(1.0f - attr.x - attr.y, attr.x, attr.y);

    payload.color = barycentrics;
}

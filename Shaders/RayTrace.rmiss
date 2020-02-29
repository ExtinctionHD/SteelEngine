#version 460
#extension GL_NV_ray_tracing : require

struct Payload
{
    vec3 color;
};

layout(location = 0) rayPayloadInNV Payload payload;

void main()
{
    payload.color = vec3(0.2f, 0.05f, 0.4f);
}
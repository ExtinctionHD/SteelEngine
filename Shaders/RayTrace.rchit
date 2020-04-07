#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require

struct Payload
{
    vec3 color;
};

struct Vertex
{
    vec3 pos;
    vec3 normal;
    vec3 tangent;
    vec2 texCoord;
};

layout(set = 2, binding = 0) readonly buffer VertexBuffers{
    Vertex vertices[];
} vertexBuffers[];

layout(set = 3, binding = 0) readonly buffer IndexBuffers{
    uint indices[];
} indexBuffers[];

layout(set = 4, binding = 0) uniform sampler2D baseColorTextures[];

layout(location = 0) rayPayloadInNV Payload payload;

hitAttributeNV vec2 attr;

vec2 Lerp(vec2 a, vec2 b, vec2 c, vec3 barycentrics)
{
    return a * barycentrics.x + b * barycentrics.y + c * barycentrics.z;
}

vec3 Lerp(vec3 a, vec3 b, vec3 c, vec3 barycentrics)
{
    return a * barycentrics.x + b * barycentrics.y + c * barycentrics.z;
}

void main()
{
    const vec3 barycentrics = vec3(1.0f - attr.x - attr.y, attr.x, attr.y);

    const uint i0 = indexBuffers[nonuniformEXT(gl_InstanceCustomIndexNV)].indices[gl_PrimitiveID * 3];
    const uint i1 = indexBuffers[nonuniformEXT(gl_InstanceCustomIndexNV)].indices[gl_PrimitiveID * 3 + 1];
    const uint i2 = indexBuffers[nonuniformEXT(gl_InstanceCustomIndexNV)].indices[gl_PrimitiveID * 3 + 2];

    Vertex v0 = vertexBuffers[nonuniformEXT(gl_InstanceCustomIndexNV)].vertices[i0];
    Vertex v1 = vertexBuffers[nonuniformEXT(gl_InstanceCustomIndexNV)].vertices[i1];
    Vertex v2 = vertexBuffers[nonuniformEXT(gl_InstanceCustomIndexNV)].vertices[i2];

    const vec2 texCoord = Lerp(v0.texCoord, v1.texCoord, v2.texCoord, barycentrics);
    const vec3 baseColor = texture(baseColorTextures[nonuniformEXT(gl_InstanceCustomIndexNV)], texCoord).rgb;

    payload.color = baseColor;
}

#version 460

layout(set = 0, binding = 1) uniform Lighting{
    vec4 lightDir;
};

layout(set = 1, binding = 1) uniform sampler2D baseColorTexture;

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

void main() 
{
    const vec4 baseColor = texture(baseColorTexture, inTexCoord).rgba;

    const vec3 N = normalize(inNormal);
    const vec3 L = normalize(-lightDir.xyz);
    
    const float cosWi = clamp(dot(N, L), 0.0f, 1.0f);

    outColor = vec4(baseColor.xyz * 0.2f + cosWi * baseColor.xyz, baseColor.a);
}
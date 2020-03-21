#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform PushConstants
{
    layout(offset = 64) vec4 colorMultiplier;
};

layout(set = 0, binding = 0) uniform sampler2D colorTexture;

layout(location = 0) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

void main() 
{
    outColor = vec4(texture(colorTexture, inTexCoord).rgb, 1.0f) * colorMultiplier;
}
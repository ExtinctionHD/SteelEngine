#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D colorTexture;

void main() 
{
    outColor = vec4(texture(colorTexture, inTexCoord).rgb, 1.0f);
}
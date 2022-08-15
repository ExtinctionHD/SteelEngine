#ifndef DEBUG_GLSL
#define DEBUG_GLSL

#ifndef SHADER_STAGE
#define SHADER_STAGE vertex
#pragma shader_stage(vertex)
void main() {}
#endif

#define DEBUG_VIEW_DIRECT_LIGHTING 1
#define DEBUG_VIEW_INDIRECT_LIGHTING 1

#define DEBUG_VIEW_DIFFUSE 1
#define DEBUG_VIEW_SPECULAR 1

#define DEBUG_OVERRIDE_MATERIAL 0
#define DEBUG_ROUGHNESS 1.0
#define DEBUG_METALLIC 0.0

#endif
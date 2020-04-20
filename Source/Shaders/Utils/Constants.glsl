#ifndef RANDOM_GLSL
#define RANDOM_GLSL

#ifndef SHADER_STAGE
#define SHADER_STAGE vertex
#pragma shader_stage(vertex)
void main() {}
#endif

#define PI 3.141592654
#define INVERSE_PI 0.31830988618

#define UNIT_X vec3(1.0f, 0.0f, 0.0f)
#define UNIT_Y vec3(0.0f, 1.0f, 0.0f)
#define UNIT_Z vec3(0.0f, 0.0f, 1.0f)

#endif
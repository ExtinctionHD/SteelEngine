#ifndef CONSTANTS_GLSL
#define CONSTANTS_GLSL

#ifndef SHADER_STAGE
#define SHADER_STAGE vertex
#pragma shader_stage(vertex)
void main() {}
#endif

#define EPSILON 0.000001

#define PI 3.141592654
#define INVERSE_PI 0.31830988618

#define UNIT_X vec3(1.0, 0.0, 0.0)
#define UNIT_Y vec3(0.0, 1.0, 0.0)
#define UNIT_Z vec3(0.0, 0.0, 1.0)

#define CUBE_FACE_COUNT 6

const vec3 CUBE_FACES_N[CUBE_FACE_COUNT] = {
    UNIT_X,
    -UNIT_X,
    UNIT_Y,
    -UNIT_Y,
    UNIT_Z,
    -UNIT_Z
};

const vec3 CUBE_FACES_T[CUBE_FACE_COUNT] = {
    -UNIT_Z,
    UNIT_Z,
    UNIT_X,
    UNIT_X,
    UNIT_X,
    -UNIT_X
};

const vec3 CUBE_FACES_B[CUBE_FACE_COUNT] = {
    -UNIT_Y,
    -UNIT_Y,
    UNIT_Z,
    -UNIT_Z,
    -UNIT_Y,
    -UNIT_Y
};

#endif
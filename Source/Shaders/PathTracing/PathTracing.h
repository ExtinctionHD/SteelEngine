#ifndef PATH_TRACING_H
#define PATH_TRACING_H

#ifdef __cplusplus
#define mat4 glm::mat4
#define vec4 glm::vec4
#define vec3 glm::vec3
#define vec2 glm::vec2
#endif

struct CameraPT
{
    mat4 inverseView;
    mat4 inverseProj;
    float zNear;
    float zFar;
};

#ifdef __cplusplus
#undef mat4
#undef vec4
#undef vec3
#undef vec2
#endif

#endif

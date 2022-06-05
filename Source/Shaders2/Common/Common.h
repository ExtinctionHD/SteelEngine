#ifndef COMMON_H
#define COMMON_H

#ifdef __cplusplus
#define mat4 glm::mat4
#define vec4 glm::vec4
#define vec3 glm::vec3
#endif

struct DynamicData
{
    mat4 viewProj;
    vec3 cameraPos;
    float globalTime;
};

struct PushData
{
    mat4 transform;
};

struct PointLight
{
    vec4 position;
    vec4 color;
};

struct DirectLight
{
    vec4 direction;
    vec4 color;
};

#ifdef __cplusplus
#undef mat4
#undef vec4
#undef vec3
#endif

#endif

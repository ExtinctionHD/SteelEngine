#ifndef COMMON_H
#define COMMON_H

#ifdef __cplusplus
#define mat4 glm::mat4
#define vec4 glm::vec4
#endif

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

struct BoundingBox
{
    vec4 min;
    vec4 max;
};

#ifdef __cplusplus
#undef mat4
#undef vec4
#endif

#endif

#ifndef COMMON_H
#define COMMON_H

#ifdef __cplusplus
#define mat4 glm::mat4
#define vec4 glm::vec4
#endif

struct DirectLight
{
    vec4 direction;
    vec4 color;
};

#ifdef __cplusplus
#undef mat4
#undef vec4
#endif

#endif

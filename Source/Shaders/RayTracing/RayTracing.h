#ifndef PATH_TRACING_H
#define PATH_TRACING_H

#ifdef __cplusplus
#define mat4 glm::mat4
#define vec4 glm::vec4
#define vec3 glm::vec3
#define vec2 glm::vec2
namespace ShaderDataRT
{
#endif
    struct Camera
    {
        mat4 inverseView;
        mat4 inverseProj;
        float zNear;
        float zFar;
    };

    struct Material
    {
        int baseColorTexture;
        int roughnessMetallicTexture;
        int normalTexture;
        int emissionTexture;

        vec4 baseColorFactor;
        vec4 emissionFactor;

        float roughnessFactor;
        float metallicFactor;
        float normalScale;
        
        float padding;
    };
#ifdef __cplusplus
}
#undef mat4
#undef vec4
#undef vec3
#undef vec2
#endif

#endif

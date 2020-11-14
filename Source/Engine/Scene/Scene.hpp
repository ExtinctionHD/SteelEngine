#pragma once
#include "Shaders/Common/Common.h"

class Camera;
struct Texture;

class Scene
{
public:
    struct Mesh
    {
        vk::IndexType indexType;
        vk::Buffer indexBuffer;
        uint32_t indexCount;

        std::vector<vk::Format> vertexFormat;
        vk::Buffer vertexBuffer;
        uint32_t vertexCount;
    };

    struct Material
    {
        int32_t baseColorTexture;
        int32_t roughnessMetallicTexture;
        int32_t normalTexture;
        int32_t occlusionTexture;
        int32_t emissionTexture;

        ShaderData::MaterialFactors factors;
        vk::Buffer factorsBuffer;
    };

    struct RenderObject
    {
        uint32_t meshIndex;
        uint32_t materialIndex;

        glm::mat4 transform;
        vk::Buffer transformBuffer;
    };

    struct Description
    {
        std::vector<Mesh> meshes;
        std::vector<Material> materials;
        std::vector<RenderObject> renderObjects;

        std::vector<Texture> textures;
        std::vector<vk::Sampler> samplers;
    };

    ~Scene();

private:
    Scene(Camera* camera_, const Description &description_);

    std::unique_ptr<Camera> camera;
    Description description;

    friend class SceneModel;
};

#pragma once

struct Texture;

class Scene
{
public:
    struct Geometry
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
        
    };

    struct RenderObject
    {
        Geometry geometry;
        glm::mat4 transform;
        uint32_t materialIndex;
    };

    struct Description
    {
        std::vector<Texture> textures;
        std::vector<vk::Sampler> samplers;
        std::vector<Material> materials;
        std::vector<RenderObject> renderObjects;
    };

    ~Scene();

private:
    Scene(const Description &description);

    friend class SceneModel;
};

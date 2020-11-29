#pragma once
#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"

class Camera;
struct Texture;

class Scene
{
public:
    struct Mesh
    {
        struct Vertex
        {
            static const std::vector<vk::Format> kFormat;

            glm::vec3 position;
            glm::vec3 normal;
            glm::vec3 tangent;
            glm::vec2 texCoord;
        };

        vk::IndexType indexType;
        vk::Buffer indexBuffer;
        uint32_t indexCount;

        vk::Buffer vertexBuffer;
        uint32_t vertexCount;
    };

    struct Material
    {
        static constexpr uint32_t kTextureCount = 5;

        int32_t baseColorTexture;
        int32_t roughnessMetallicTexture;
        int32_t normalTexture;
        int32_t occlusionTexture;
        int32_t emissionTexture;

        vk::Buffer factorsBuffer;
    };

    struct RenderObject
    {
        uint32_t meshIndex;
        uint32_t materialIndex;
        glm::mat4 transform;
    };

    struct Hierarchy
    {
        std::vector<Mesh> meshes;
        std::vector<Material> materials;
        std::vector<RenderObject> renderObjects;
    };

    struct Resources
    {
        std::vector<vk::Buffer> buffers;
        std::vector<vk::Sampler> samplers;
        std::vector<Texture> textures;
    };

    struct DescriptorSets
    {
        MultiDescriptorSet materials;
    };

    struct Description
    {
        Hierarchy hierarchy;
        Resources resources;
        DescriptorSets descriptorSets;
    };

    ~Scene();

    const DescriptorSets& GetDescriptorSets() const { return description.descriptorSets; }

private:
    Scene(const Description& description_);

    Description description;

    friend class SceneModel;
};

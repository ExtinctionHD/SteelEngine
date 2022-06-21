#pragma once

#include <entt/entity/registry.hpp>

#include "Engine2/Material.hpp"
#include "Engine2/Primitive.hpp"

#include "Engine/Filesystem/Filepath.hpp"
#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"

#include "Utils/AABBox.hpp"

struct Texture;

class Scene2 : public entt::registry
{
public:
    struct MaterialTexture
    {
        vk::ImageView view;
        vk::Sampler sampler;
    };

    Scene2(const Filepath& path);
    ~Scene2();

    AABBox bbox;
    
    std::vector<Texture> textures;
    std::vector<vk::Sampler> samplers;
    std::vector<MaterialTexture> materialTextures;

    std::vector<Primitive> primitives;
    std::vector<Material> materials;

    vk::Buffer materialBuffer;

    vk::AccelerationStructureKHR tlas;

    void AddScene(Scene2&& scene, entt::entity parent);

    void PrepareToRender();
};

namespace SceneHelpers
{
    DescriptorData GetDescriptorData(const std::vector<Scene2::MaterialTexture>& textures);
}
#pragma once

#include <entt/entity/registry.hpp>

#include "Engine/Filesystem/Filepath.hpp"

struct Texture;
struct Material;
struct Primitive;

class Scene2 : public entt::registry
{
public:
    struct MaterialTexture
    {
        vk::ImageView view;
        vk::Sampler sampler;
    };

    Scene2();
    ~Scene2();
    
    std::vector<Texture> textures;
    std::vector<vk::Sampler> samplers;
    std::vector<MaterialTexture> materialTextures;

    std::vector<Primitive> primitives;
    std::vector<Material> materials;

    void Load(const Filepath& path);
};


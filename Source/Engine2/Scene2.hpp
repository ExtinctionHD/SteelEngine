#pragma once

#include <entt/entity/registry.hpp>

#include "RenderComponent.hpp"
#include "Engine/Filesystem/Filepath.hpp"

struct Texture;

namespace Steel
{
    struct Scene2
    {
        struct MaterialTexture
        {
            vk::ImageView view;
            vk::Sampler sampler;
        };
        
        entt::registry registry;
        
        std::vector<Texture> textures;
        std::vector<vk::Sampler> samplers;
        std::vector<MaterialTexture> materialTextures;
        std::vector<Material> materials;
        std::vector<Geometry> geometry;

        void Load(const Filepath& path);
    };
}

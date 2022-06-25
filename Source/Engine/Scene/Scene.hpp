#pragma once

#include <entt/entity/registry.hpp>

#include "Engine/Scene/Material.hpp"
#include "Engine/Scene/Primitive.hpp"

#include "Engine/Filesystem/Filepath.hpp"
#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"

#include "Utils/AABBox.hpp"

struct Texture;

class Scene : public entt::registry
{
public:

    Scene(const Filepath& path);
    ~Scene();

    AABBox bbox;

    std::vector<Texture> images;
    std::vector<vk::Sampler> samplers;
    std::vector<SampledTexture> textures;

    std::vector<Primitive> primitives;
    std::vector<Material> materials;

    vk::Buffer materialBuffer;
    vk::AccelerationStructureKHR tlas;

    void AddScene(Scene&& scene, entt::entity parent);

    void PrepareToRender();
};
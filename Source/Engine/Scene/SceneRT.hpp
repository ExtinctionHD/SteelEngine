#pragma once

#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"

class SceneModel;

class SceneRT
{
public:

private:
    vk::AccelerationStructureNV tlas;

    std::vector<Texture> textures;
    std::vector<vk::Sampler> samplers;

    friend class SceneModel;
};

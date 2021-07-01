#pragma once

#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"
#include "Utils/AABBox.hpp"

class ScenePT;
class Environment;

struct IrradianceVolume
{
    struct Point
    {
        glm::vec3 position;
        glm::uvec3 coord;
    };

    AABBox bbox;
    std::vector<Point> points;
    std::vector<Texture> textures;
};

class GlobalIllumination
{
public:
    GlobalIllumination();
    ~GlobalIllumination();

    vk::Sampler GetIrradianceVolumeSampler() const { return irradianceVolumeSampler; }

    IrradianceVolume GenerateIrradianceVolume(ScenePT* scene, Environment* environment, const AABBox& bbox) const;

private:
    vk::Sampler irradianceVolumeSampler;

    vk::DescriptorSetLayout probeLayout;
    vk::DescriptorSetLayout texturesLayout;

    std::unique_ptr<ComputePipeline> irradianceVolumePipeline;
};

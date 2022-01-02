#pragma once

#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"

#include "Utils/AABBox.hpp"

class ScenePT;
class Environment;

struct IrradianceVolume
{
    struct Vertex
    {
        glm::vec3 position;
        glm::uvec3 coord;
    };

    AABBox bbox;
    std::vector<Vertex> vertices;
    std::vector<Texture> textures;
};

struct LightVolume
{
    vk::Buffer positionsBuffer;
    vk::Buffer tetrahedralBuffer;
    vk::Buffer coefficientsBuffer;
    std::vector<glm::vec3> positions;
};

class GlobalIllumination
{
public:
    GlobalIllumination();
    ~GlobalIllumination();

    vk::Sampler GetIrradianceVolumeSampler() const { return irradianceVolumeSampler; }

    IrradianceVolume GenerateIrradianceVolume(ScenePT* scene, Environment* environment) const;

    LightVolume GenerateLightVolume(ScenePT* scene, Environment* environment) const;

private:
    vk::Sampler irradianceVolumeSampler;

    vk::DescriptorSetLayout probeLayout;
    vk::DescriptorSetLayout texturesLayout;
    vk::DescriptorSetLayout coefficientsLayout;

    std::unique_ptr<ComputePipeline> irradianceVolumePipeline;
    std::unique_ptr<ComputePipeline> lightVolumePipeline;
};

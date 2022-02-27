#pragma once

#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"

class Scene;
class ScenePT;
class Environment;

struct LightVolume
{
    vk::Buffer positionsBuffer;
    vk::Buffer tetrahedralBuffer;
    vk::Buffer coefficientsBuffer;
    std::vector<glm::vec3> positions;
    std::vector<uint32_t> edgeIndices;
};

class GlobalIllumination
{
public:
    GlobalIllumination();
    ~GlobalIllumination();

    LightVolume GenerateLightVolume(const Scene* scene,
            const ScenePT* scenePT, const Environment* environment) const;

private:
    vk::DescriptorSetLayout probeLayout;
    vk::DescriptorSetLayout coefficientsLayout;

    std::unique_ptr<ComputePipeline> lightVolumePipeline;
};

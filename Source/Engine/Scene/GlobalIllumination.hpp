#pragma once

class Scene;
class ComputePipeline;
class DescriptorProvider;

struct LightVolumeComponent
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

    LightVolumeComponent GenerateLightVolume(const Scene& scene) const;

private:
    std::unique_ptr<ComputePipeline> lightVolumePipeline;

    std::unique_ptr<DescriptorProvider> descriptorProvider;
};

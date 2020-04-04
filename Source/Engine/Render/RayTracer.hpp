#pragma once

#include "Engine/Render/Renderer.hpp"

class Scene;
class Camera;
class RenderObject;
class RayTracingPipeline;
struct GeometryInstance;

class RayTracer
        : public Renderer
{
public:
    RayTracer(Scene &scene_, Camera &camera_);

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) override;

    void OnResize(const vk::Extent2D &extent) override;

private:
    struct RenderObjectUniforms
    {
        vk::AccelerationStructureNV blas;
        const glm::mat4 &transform;
    };

    struct GlobalUniforms
    {
        vk::DescriptorSet descriptorSet;
        vk::AccelerationStructureNV tlas;
        vk::Buffer cameraBuffer;
    };

    Scene &scene;
    Camera &camera;

    std::map<const RenderObject*, RenderObjectUniforms> renderObjects;

    vk::DescriptorSetLayout renderTargetLayout;
    std::vector<vk::DescriptorSet> renderTargets;

    vk::DescriptorSetLayout globalLayout;
    GlobalUniforms globalUniforms;

    std::unique_ptr<RayTracingPipeline> rayTracingPipeline;

    void SetupRenderTargets();
    void SetupRenderObjects();
    void SetupGlobalUniforms();

    std::vector<GeometryInstance> CollectGeometryInstances();

    void TraceRays(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;
};

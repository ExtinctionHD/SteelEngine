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
    struct CameraData
    {
        glm::mat4 inverseView;
        glm::mat4 inverseProj;
        float zNear;
        float zFar;
    };

    struct GlobalUniforms
    {
        vk::DescriptorSet descriptorSet;
        vk::AccelerationStructureNV tlas;
        vk::Buffer cameraBuffer;
    };

    struct IndexedDescriptor
    {
        vk::DescriptorSetLayout layout;
        vk::DescriptorSet descriptorSet;
    };

    struct IndexedUniforms
    {
        IndexedDescriptor vertexBuffers;
        IndexedDescriptor indexBuffers;
        IndexedDescriptor baseColorTextures;
    };

    Scene &scene;
    Camera &camera;

    vk::DescriptorSetLayout renderTargetLayout;
    std::vector<vk::DescriptorSet> renderTargets;

    vk::DescriptorSetLayout globalLayout;
    GlobalUniforms globalUniforms;

    IndexedUniforms indexedUniforms;

    std::unique_ptr<RayTracingPipeline> rayTracingPipeline;

    void SetupRenderTarget();
    void SetupGlobalUniforms();
    void SetupIndexedUniforms();

    void TraceRays(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;
};

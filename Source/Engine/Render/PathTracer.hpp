#pragma once

#include "Engine/Render/Renderer.hpp"
#include "Engine/Render/Vulkan/RayTracing/AccelerationStructureHelpers.hpp"
#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/Texture.hpp"

class Scene;
class Camera;
class RenderObject;
class RayTracingPipeline;

class PathTracer
        : public Renderer
{
public:
    PathTracer(Scene &scene_, Camera &camera_);

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) override;

    void OnResize(const vk::Extent2D &extent) override;

    void ResetAccumulation();

private:
    struct RenderObjectEntry
    {
        vk::Buffer vertexBuffer;
        vk::Buffer indexBuffer;
        vk::Buffer materialBuffer;
        GeometryInstance geometryInstance;
    };

    struct GlobalUniforms
    {
        vk::DescriptorSet descriptorSet;
        vk::AccelerationStructureNV tlas;
        vk::Buffer cameraBuffer;
        vk::Buffer lightingBuffer;
        Texture environmentMap;
    };

    struct IndexedUniforms
    {
        IndexedDescriptor vertexBuffers;
        IndexedDescriptor indexBuffers;
        IndexedDescriptor materialBuffers;
        IndexedDescriptor baseColorTextures;
        IndexedDescriptor surfaceTextures;
        IndexedDescriptor normalTextures;
    };

    Scene &scene;
    Camera &camera;

    std::unordered_map<const RenderObject*, RenderObjectEntry> renderObjects;

    vk::DescriptorSetLayout renderTargetLayout;
    std::vector<vk::DescriptorSet> renderTargets;

    vk::DescriptorSetLayout globalLayout;
    GlobalUniforms globalUniforms;

    std::vector<vk::Buffer> vertexBuffers;
    std::vector<vk::Buffer> indexBuffers;
    IndexedUniforms indexedUniforms;

    std::unique_ptr<RayTracingPipeline> rayTracingPipeline;

    uint32_t accumulationIndex = 1;

    void SetupRenderTarget();
    void SetupGlobalUniforms();
    void SetupIndexedUniforms();

    void SetupRenderObject(const RenderObject &renderObject, const glm::mat4 &transform);

    void TraceRays(vk::CommandBuffer commandBuffer, uint32_t imageIndex);
};

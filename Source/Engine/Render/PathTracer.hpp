#pragma once

#include "Engine/Render/Renderer.hpp"
#include "Engine/Render/Vulkan/RayTracing/AccelerationStructureHelpers.hpp"
#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"

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

private:
    struct RenderObjectEntry
    {
        vk::Buffer vertexBuffer;
        vk::Buffer indexBuffer;
        GeometryInstance geometryInstance;
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

        void Create(const std::variant<BufferInfo, ImageInfo> &info);
    };

    struct IndexedUniforms
    {
        IndexedDescriptor vertexBuffers;
        IndexedDescriptor indexBuffers;
        IndexedDescriptor baseColorTextures;
        IndexedDescriptor surfaceTextures;
        IndexedDescriptor occlusionTextures;
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

    void SetupRenderTarget();
    void SetupGlobalUniforms();
    void SetupIndexedUniforms();

    void SetupRenderObject(const RenderObject &renderObject, const glm::mat4 &transform);

    void TraceRays(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;
};

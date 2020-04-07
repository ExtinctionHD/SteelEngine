#pragma once

#include "Engine/Render/Renderer.hpp"
#include "Engine/Render/Vulkan/RayTracing/AccelerationStructureManager.hpp"

class Scene;
class Camera;
class RenderObject;
class RayTracingPipeline;

struct CameraData
{
    glm::mat4 inverseView;
    glm::mat4 inverseProj;
    float zNear;
    float zFar;
};

struct VertexData
{
    glm::vec4 pos;
    glm::vec4 normal;
    glm::vec4 tangent;
    glm::vec4 texCoord;
};

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
    };

    struct IndexedUniforms
    {
        IndexedDescriptor vertexBuffers;
        IndexedDescriptor indexBuffers;
        IndexedDescriptor baseColorTextures;
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

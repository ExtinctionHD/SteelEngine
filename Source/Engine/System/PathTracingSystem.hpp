#pragma once

#include "Engine/Render/Vulkan/RayTracing/AccelerationStructureHelpers.hpp"
#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/Texture.hpp"
#include "Engine/Render/Vulkan/ComputePipeline.hpp"
#include "Engine/System/System.hpp"

class Scene;
class Camera;
class RenderObject;
class RayTracingPipeline;

class PathTracingSystem
        : public System
{
public:
    PathTracingSystem(Scene *scene_, Camera *camera_);

    void Process(float deltaSeconds) override;

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

private:
    struct RenderTargets
    {
        vk::DescriptorSetLayout layout;
        std::vector<vk::DescriptorSet> descriptorSets;
    };

    struct AccumulationTarget
    {
        vk::Image image;
        vk::ImageView view;
        vk::DescriptorSetLayout layout;
        vk::DescriptorSet descriptorSet;
    };

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

    Scene *scene;
    Camera *camera;

    std::unordered_map<const RenderObject*, RenderObjectEntry> renderObjects;

    RenderTargets renderTargets;
    AccumulationTarget accumulationTarget;

    vk::DescriptorSetLayout globalLayout;
    GlobalUniforms globalUniforms;

    std::vector<vk::Buffer> vertexBuffers;
    std::vector<vk::Buffer> indexBuffers;
    IndexedUniforms indexedUniforms;

    std::unique_ptr<RayTracingPipeline> rayTracingPipeline;
    std::unique_ptr<ComputePipeline> copyingPipeline;

    uint32_t accumulationIndex = 0;

    void SetupRenderTargets();
    void SetupAccumulationTarget();
    void SetupGlobalUniforms();
    void SetupIndexedUniforms();

    void SetupRenderObject(const RenderObject &renderObject, const glm::mat4 &transform);

    void TraceRays(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

    void HandleResizeEvent(const vk::Extent2D &extent);

    void ResetAccumulation();
};

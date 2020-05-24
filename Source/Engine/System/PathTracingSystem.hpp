#pragma once

#include "Engine/Render/Vulkan/RayTracing/AccelerationStructureHelpers.hpp"
#include "Engine/Render/Vulkan/DescriptorHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"
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
    ~PathTracingSystem();

    void Process(float deltaSeconds) override;

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

private:
    struct RenderObjectEntry
    {
        vk::Buffer vertexBuffer;
        vk::Buffer indexBuffer;
        vk::Buffer materialBuffer;
        GeometryInstance geometryInstance;
    };

    struct RenderTargets
    {
        MultiDescriptorSet multiDescriptor;
    };

    struct AccumulationTarget
    {
        vk::Image image;
        vk::ImageView view;
        DescriptorSet descriptor;
    };

    struct GlobalUniforms
    {
        vk::AccelerationStructureNV tlas;
        vk::Buffer cameraBuffer;
        vk::Buffer lightingBuffer;
        Texture environmentMap;
        DescriptorSet descriptorSet;
    };

    struct IndexedUniforms
    {
        std::vector<vk::Buffer> vertexBuffers;
        std::vector<vk::Buffer> indexBuffers;
        DescriptorSet vertexBuffersDescriptor;
        DescriptorSet indexBuffersDescriptor;
        DescriptorSet materialBuffersDescriptor;
        DescriptorSet baseColorTexturesDescriptor;
        DescriptorSet surfaceTexturesDescriptor;
        DescriptorSet normalTexturesDescriptor;
    };

    Scene *scene;
    Camera *camera;

    std::unordered_map<const RenderObject*, RenderObjectEntry> renderObjects;

    RenderTargets renderTargets;
    AccumulationTarget accumulationTarget;
    GlobalUniforms globalUniforms;
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

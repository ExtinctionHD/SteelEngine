#pragma once

#include "Engine/Render/RenderHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/DescriptorProvider.hpp"

class Scene;
class Scene;
class RenderPass;
class GraphicsPipeline;
struct KeyInput;

class ForwardStage
{
public:
    ForwardStage(vk::ImageView depthImageView);

    ~ForwardStage();

    void RegisterScene(const Scene* scene_);

    void RemoveScene();

    void Execute(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void Resize(vk::ImageView depthImageView);

    void ReloadShaders();

private:
    struct EnvironmentData
    {
        vk::Buffer indexBuffer;
    };

    struct LightVolumeData
    {
        uint32_t positionsIndexCount = 0;
        uint32_t positionsInstanceCount = 0;
        uint32_t edgesIndexCount = 0;

        vk::Buffer positionsIndexBuffer;
        vk::Buffer positionsVertexBuffer;
        vk::Buffer positionsInstanceBuffer;
        vk::Buffer edgesIndexBuffer;
    };

    static EnvironmentData CreateEnvironmentData();
    static LightVolumeData CreateLightVolumeData(const Scene& scene);

    const Scene* scene = nullptr;

    std::unique_ptr<RenderPass> renderPass;
    std::vector<vk::Framebuffer> framebuffers;

    CameraData defaultCameraData;
    CameraData environmentCameraData;

    EnvironmentData environmentData;
    LightVolumeData lightVolumeData;

    // TODO:
    // Check that all pipelines have common descriptorSetLayouts
    // Implement DescriptorSetLayout cache inside DescriptorManager
    
    std::vector<MaterialPipeline> materialPipelines;
    std::unique_ptr<GraphicsPipeline> environmentPipeline;
    std::unique_ptr<GraphicsPipeline> lightVolumePositionsPipeline;
    std::unique_ptr<GraphicsPipeline> lightVolumeEdgesPipeline;

    FrameDescriptorProvider materialsDescriptorProvider;
    FrameDescriptorProvider environmentDescriptorProvider;
    FrameDescriptorProvider lightVolumePositionsDescriptorProvider;
    FrameOnlyDescriptorProvider lightVolumeEdgesDescriptorProvider;

    bool drawLightVolume = false;

    void CreateMaterialsDescriptorProvider();
    void CreateEnvironmentDescriptorProvider();
    void CreateLightVolumePositionsDescriptorProvider();
    void CreateLightVolumeEdgesDescriptorProvider();

    // TODO rename
    void DrawTranslucency(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;
    void DrawEnvironment(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const; // TODO draw env as regular RO
    void DrawLightVolume(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void HandleKeyInputEvent(const KeyInput& keyInput);
};

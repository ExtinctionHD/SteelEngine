#pragma once

#include "Engine/Render/RenderHelpers.hpp"
#include "Engine/Scene/Material.hpp"

class Scene;
class RenderPass;
class GraphicsPipeline;

class GBufferStage
{
public:
    static constexpr std::array<vk::Format, 5> kFormats{
        vk::Format::eA2B10G10R10UnormPack32, // normals
        vk::Format::eB10G11R11UfloatPack32,  // emission
        vk::Format::eR8G8B8A8Unorm,          // albedo + occlusion
        vk::Format::eR8G8Unorm,              // roughness + metallic
        vk::Format::eD32Sfloat               // depth
    };

    static constexpr vk::Format kDepthFormat = kFormats.back();

    GBufferStage(const Scene* scene_, const std::vector<vk::ImageView>& imageViews);

    ~GBufferStage();

    void Execute(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void Resize(const std::vector<vk::ImageView>& imageViews);

    void ReloadShaders();

private:
    struct MaterialPipeline
    {
        MaterialFlags materialFlags;
        std::unique_ptr<GraphicsPipeline> pipeline;
    };
    
    const Scene* scene = nullptr;

    std::unique_ptr<RenderPass> renderPass;
    vk::Framebuffer framebuffer;

    DescriptorSet materialDescriptorSet;
    CameraData cameraData;

    std::vector<MaterialPipeline> pipelines;

    void SetupCameraData();

    void SetupMaterialsData();

    void SetupPipelines();

    void DrawScene(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;
};

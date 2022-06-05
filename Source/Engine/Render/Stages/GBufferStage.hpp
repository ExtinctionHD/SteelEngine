#pragma once

#include "Engine/Render/RenderHelpers.hpp"
#include "Engine/Scene/Scene.hpp"
#include "Engine2/Scene2.hpp"

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

    GBufferStage(const Scene2* scene_, const Camera* camera_, 
        const std::vector<vk::ImageView>& imageViews);

    ~GBufferStage();

    void Execute(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void Resize(const std::vector<vk::ImageView>& imageViews);

    void ReloadShaders();

private:
    struct MaterialsData
    {
        vk::Buffer buffer;
        DescriptorSet descriptorSet;
    };

    struct MaterialPipeline
    {
        Scene2::MaterialFlags materialFlags;
        std::unique_ptr<GraphicsPipeline> pipeline;
    };
    
    const Scene2* scene = nullptr;
    const Camera* camera = nullptr;

    std::unique_ptr<RenderPass> renderPass;
    vk::Framebuffer framebuffer;

    MaterialsData materialsData;
    CameraData cameraData;

    std::vector<MaterialPipeline> pipelines;

    void SetupCameraData();

    void SetupMaterialsData();

    void SetupPipelines();
};

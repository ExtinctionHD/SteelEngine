#pragma once

#include "Engine/Render/RenderHelpers.hpp"
#include "Engine/Scene/Material.hpp"

class Scene;
class RenderPass;
class GraphicsPipeline;
struct Texture;

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

    static constexpr uint32_t kAttachmentCount = static_cast<uint32_t>(kFormats.size());

    static constexpr uint32_t kColorAttachmentCount = kAttachmentCount - 1;

    GBufferStage();

    ~GBufferStage();

    std::vector<vk::ImageView> GetImageViews() const;

    vk::ImageView GetDepthImageView() const;

    void RegisterScene(const Scene* scene_);

    void RemoveScene();

    void Execute(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void Resize();

    void ReloadShaders();

private:
    struct MaterialPipeline
    {
        MaterialFlags materialFlags;
        std::unique_ptr<GraphicsPipeline> pipeline;
    };

    static std::vector<MaterialPipeline> CreateMaterialPipelines(
            const Scene& scene, const RenderPass& renderPass,
            const std::vector<vk::DescriptorSetLayout>& layouts);

    const Scene* scene = nullptr;

    std::unique_ptr<RenderPass> renderPass;
    std::vector<Texture> renderTargets;
    vk::Framebuffer framebuffer;

    CameraData cameraData;
    DescriptorSet materialDescriptorSet;
    std::vector<MaterialPipeline> materialPipelines;

    std::vector<vk::DescriptorSetLayout> GetDescriptorSetLayouts() const;

    void DrawScene(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;
};

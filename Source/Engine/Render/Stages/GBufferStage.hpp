#pragma once

#include "Engine/Render/RenderHelpers.hpp"
#include "Engine/Render/Vulkan/Resources/ImageHelpers.hpp"

class Scene;
class RenderPass;
class PipelineCache;
class DescriptorProvider;

class GBufferStage
{
public:
    static constexpr std::array<vk::Format, 5> kFormats{
        vk::Format::eA2B10G10R10UnormPack32, // normals
        vk::Format::eB10G11R11UfloatPack32,  // emission
        vk::Format::eR8G8B8A8Unorm,          // baseColor + occlusion
        vk::Format::eR8G8Unorm,              // roughness + metallic
        vk::Format::eD32Sfloat               // depth
    };

    static constexpr vk::Format kDepthFormat = kFormats.back();

    static constexpr uint32_t kAttachmentCount = static_cast<uint32_t>(kFormats.size());

    static constexpr uint32_t kColorAttachmentCount = kAttachmentCount - 1;

    GBufferStage();

    ~GBufferStage();

    const std::vector<RenderTarget>& GetRenderTargets() const { return renderTargets; }

    const RenderTarget& GetDepthTarget() const { return renderTargets.back(); }

    void RegisterScene(const Scene* scene_);

    void RemoveScene();

    void Update();

    void Execute(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void Resize();

    void ReloadShaders() const;

private:
    const Scene* scene = nullptr;

    std::unique_ptr<RenderPass> renderPass;
    std::unique_ptr<PipelineCache> pipelineCache;
    std::set<MaterialFlags> uniquePipelines;

    std::vector<RenderTarget> renderTargets;
    vk::Framebuffer framebuffer;

    void DrawScene(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;
};

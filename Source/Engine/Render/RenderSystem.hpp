#pragma once

#include "Engine/System.hpp"
#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

class Camera;
class Scene;
class ForwardRenderPass;
class PathTracer;

using RenderFunction = std::function<void(vk::CommandBuffer, uint32_t)>;

enum class RenderFlow
{
    eForward,
    ePathTracing
};

class RenderSystem
        : public System
{
public:
    RenderSystem(Scene &scene_, Camera &camera_,
            const RenderFunction &uiRenderFunction_);
    ~RenderSystem();

    void Process(float deltaSeconds, EngineState &engineState) override;

    void OnResize(const vk::Extent2D &extent) override;

private:
    struct Frame
    {
        vk::CommandBuffer commandBuffer;
        CommandBufferSync sync;
    };

    uint32_t frameIndex = 0;
    std::vector<Frame> frames;

    RenderFlow renderFlow = RenderFlow::ePathTracing;
    std::unique_ptr<ForwardRenderPass> forwardRenderPass;
    std::unique_ptr<PathTracer> pathTracer;

    RenderFunction uiRenderFunction;

    bool drawingSuspended = true;

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;
};

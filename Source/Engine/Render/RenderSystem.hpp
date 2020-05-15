#pragma once

#include "Engine/System.hpp"
#include "Engine/Render/Vulkan/VulkanHelpers.hpp"

class Camera;
class Scene;
class ForwardRenderPass;
class PathTracer;

class RenderSystem
        : public System
{
public:
    RenderSystem(Scene *scene_, Camera *camera_);
    ~RenderSystem();

    void Process(float deltaSeconds) override;

private:
    struct Frame
    {
        vk::CommandBuffer commandBuffer;
        CommandBufferSync sync;
    };

    uint32_t frameIndex = 0;
    std::vector<Frame> frames;

    std::unique_ptr<PathTracer> pathTracer;

    bool drawingSuspended = false;

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) const;

    void HandleResizeEvent(const vk::Extent2D &extent);

    void HandleCameraUpdateEvent() const;
};

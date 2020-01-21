#pragma once

#include "Engine/Render/Vulkan/Device.hpp"

class ForwardRenderPass;

class RenderLoop
{
public:
    RenderLoop(std::shared_ptr<Device> device, uint32_t frameCount);
    ~RenderLoop();

    void ProcessNextFrame(const ForwardRenderPass &renderPass);

private:
    struct FrameData
    {
        vk::CommandBuffer commandBuffer;
        vk::Semaphore presentCompleteSemaphore;
        vk::Semaphore renderCompleteSemaphore;
    };

    std::shared_ptr<Device> device;

    std::vector<FrameData> frames;
};

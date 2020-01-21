#pragma once

#include "Engine/Render/Vulkan/Device.hpp"
#include "Engine/Render/RenderObject.hpp"

class ForwardRenderPass
{
public:
    ForwardRenderPass(std::shared_ptr<Device> device, uint32_t frameCount);
    ~ForwardRenderPass();

    void Begin(uint32_t frameIndex);

    void Draw();

    void End();

private:
    std::vector<RenderObject> renderObjects;
};

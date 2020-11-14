#pragma once
#include "Engine/System/System.hpp"

struct KeyInput;
class Scene;

class RenderSystem
    : public System
{
public:
    RenderSystem(Scene* scene_);
    ~RenderSystem();

    void Process(float deltaSeconds) override;

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex);

private:
    Scene* scene = nullptr;

    void HandleResizeEvent(const vk::Extent2D& extent);

    void HandleKeyInputEvent(const KeyInput& keyInput);

    void ReloadShaders();
};

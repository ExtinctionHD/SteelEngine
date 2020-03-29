#pragma once

#include "Engine/Render/Renderer.hpp"

class Scene;
class Camera;

class Rasterizer
        : public Renderer
{
public:
    Rasterizer(Scene &scene_, Camera &camera_);

    void Render(vk::CommandBuffer commandBuffer, uint32_t imageIndex) override;

    void OnResize(const vk::Extent2D &extent) override;

private:
    Scene &scene;
    Camera &camera;
};

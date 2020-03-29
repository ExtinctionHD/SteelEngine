#include "Engine/Render/Rasterizer.hpp"

Rasterizer::Rasterizer(Scene &scene_, Camera &camera_)
    : scene(scene_)
    , camera(camera_)
{ }

void Rasterizer::Render(vk::CommandBuffer, uint32_t)
{ }

void Rasterizer::OnResize(const vk::Extent2D &)
{ }

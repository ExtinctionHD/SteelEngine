#include "Engine/Render/RayTracer.hpp"

RayTracer::RayTracer(Scene &scene_, Camera &camera_)
    : scene(scene_)
    , camera(camera_)
{}

void RayTracer::Render(vk::CommandBuffer, uint32_t)
{}

void RayTracer::OnResize(const vk::Extent2D &)
{}

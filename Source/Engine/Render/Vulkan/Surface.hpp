#pragma once

#include "Engine/Render/Vulkan/Instance.hpp"

struct GLFWwindow;

class Surface
{
public:
    static std::unique_ptr<Surface> Create(std::shared_ptr<Instance> instance, GLFWwindow *window);

    Surface(std::shared_ptr<Instance> aInstance, vk::SurfaceKHR aSurface);
    ~Surface();

    vk::SurfaceKHR Get() const { return surface; }

private:
    std::shared_ptr<Instance> instance;

    vk::SurfaceKHR surface;
};

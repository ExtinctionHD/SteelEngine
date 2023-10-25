#pragma once

struct GLFWwindow;

class Surface
{
public:
    static std::unique_ptr<Surface> Create(GLFWwindow* window);
    ~Surface();

    vk::SurfaceKHR Get() const
    {
        return surface;
    }

private:
    vk::SurfaceKHR surface;

    Surface(vk::SurfaceKHR surface_);
};

#pragma once

#include <GLFW/glfw3.h>

namespace vk
{
    struct Extent2D;
}

class Window
{
public:
    enum class eMode
    {
        kWindowed,
        kBorderless,
        kFullscreen
    };

    Window(const vk::Extent2D &extent, eMode mode);
    ~Window();

    GLFWwindow *Get() const { return window; }

    vk::Extent2D GetExtent() const;

    bool ShouldClose() const;

    void PollEvents() const;

private:
    GLFWwindow *window;

    static void ErrorCallback(int code, const char *description);
};

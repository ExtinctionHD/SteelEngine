#pragma once

struct GLFWwindow;

enum class WindowMode
{
    eWindowed,
    eBorderless,
    eFullscreen
};

class Window
{
public:
    Window(const vk::Extent2D &extent, WindowMode mode);
    ~Window();

    GLFWwindow *Get() const { return window; }

    vk::Extent2D GetExtent() const;

    bool ShouldClose() const;

    void PollEvents() const;

private:
    GLFWwindow *window;
};

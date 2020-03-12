#pragma once

enum class WindowMode
{
    eWindowed,
    eBorderless,
    eFullscreen
};

using ResizeCallback = std::function<void(const vk::Extent2D &)>;

class Window
{
public:
    Window(const vk::Extent2D &extent, WindowMode mode);
    ~Window();

    GLFWwindow *Get() const { return window; }

    vk::Extent2D GetExtent() const;

    void SetResizeCallback(ResizeCallback resizeCallback_);

    bool ShouldClose() const;

    void PollEvents() const;

private:
    GLFWwindow *window;

    ResizeCallback resizeCallback;
};

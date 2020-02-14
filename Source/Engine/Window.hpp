#pragma once

enum class eWindowMode
{
    kWindowed,
    kBorderless,
    kFullscreen
};

using ResizeCallback = std::function<void(const vk::Extent2D &)>;

class Window
{
public:
    Window(const vk::Extent2D &extent, eWindowMode mode);
    ~Window();

    GLFWwindow *Get() const { return window; }

    vk::Extent2D GetExtent() const;

    void SetResizeCallback(ResizeCallback aResizeCallback);

    bool ShouldClose() const;

    void PollEvents() const;

private:
    GLFWwindow *window;

    ResizeCallback resizeCallback;
};

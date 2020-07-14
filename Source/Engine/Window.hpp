#pragma once

struct GLFWwindow;

class Window
{
public:
    enum class Mode
    {
        eWindowed,
        eBorderless,
        eFullscreen
    };

    Window(const vk::Extent2D &extent, Mode mode);
    ~Window();

    GLFWwindow *Get() const { return window; }

    vk::Extent2D GetExtent() const;

    bool ShouldClose() const;

    void PollEvents() const;

private:
    GLFWwindow *window;
};

#pragma once

enum class eWindowMode
{
    kWindowed,
    kBorderless,
    kFullscreen
};

class Window
{
public:

    Window(const vk::Extent2D &extent, eWindowMode mode);
    ~Window();

    GLFWwindow *Get() const { return window; }

    vk::Extent2D GetExtent() const;

    bool ShouldClose() const;

    void PollEvents() const;

private:
    GLFWwindow *window;

    static void ErrorCallback(int code, const char *description);
};

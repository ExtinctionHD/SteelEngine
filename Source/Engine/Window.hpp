#pragma once

#include "GLFW/glfw3.h"

class Window
{
public:
    enum class eMode
    {
        kWindowed,
        kBorderless,
        kFullscreen
    };

    Window(int width, int height, eMode mode);
    ~Window();

    bool ShouldClose() const;

    void PollEvents() const;

private:
    GLFWwindow *window;

    static void ErrorCallback(int code, const char *description);
};

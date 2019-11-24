#include "Engine/Window.hpp"

#include "Utils/Assert.hpp"

#include <iostream>

Window::Window(int width, int height, eMode mode)
{
    glfwSetErrorCallback(&Window::ErrorCallback);

    Assert(glfwInit());

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    GLFWmonitor *monitor = nullptr;
    switch (mode)
    {
    case eMode::kWindowed:
        break;
    case eMode::kBorderless:
        glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        break;
    case eMode::kFullscreen:
        monitor = glfwGetPrimaryMonitor();
        break;
    }

    window = glfwCreateWindow(width, height, "VulkanRayTracing", monitor, nullptr);
    Assert(window != nullptr);
}

Window::~Window()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

bool Window::ShouldClose() const
{
    return glfwWindowShouldClose(window);
}

void Window::PollEvents() const
{
    glfwPollEvents();
}

void Window::ErrorCallback(int code, const char *description)
{
    std::cout << "[GLFW] error " << code << " occured: " << description << std::endl;
}

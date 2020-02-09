#include "Engine/Window.hpp"

#include "Engine/Config.hpp"
#include "Utils/Assert.hpp"

Window::Window(const vk::Extent2D &extent, eWindowMode mode)
{
    glfwSetErrorCallback(&Window::ErrorCallback);

    Assert(glfwInit());

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    GLFWmonitor *monitor = nullptr;
    switch (mode)
    {
    case eWindowMode::kWindowed:
        break;
    case eWindowMode::kBorderless:
        glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        break;
    case eWindowMode::kFullscreen:
        monitor = glfwGetPrimaryMonitor();
        break;
    default:
        Assert(false);
        break;
    }

    window = glfwCreateWindow(extent.width, extent.height, Config::kEngineName, monitor, nullptr);
    Assert(window != nullptr);
}

Window::~Window()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

vk::Extent2D Window::GetExtent() const
{
    int width, height;

    glfwGetFramebufferSize(window, &width, &height);

    return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
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
    std::cout << "[GLFW] Error " << code << " occured: " << description << std::endl;
}

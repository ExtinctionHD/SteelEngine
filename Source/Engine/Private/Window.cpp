#include "Engine/Window.hpp"

#include "Engine/Config.hpp"
#include "Utils/Assert.hpp"

Window::Window(const vk::Extent2D &extent, WindowMode mode)
{
    glfwSetErrorCallback([](int code, const char *description)
        {
            std::cout << "[GLFW] Error " << code << " occured: " << description << std::endl;
        });

    Assert(glfwInit());

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    GLFWmonitor *monitor = nullptr;
    switch (mode)
    {
    case WindowMode::eWindowed:
        break;
    case WindowMode::eBorderless:
        glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        break;
    case WindowMode::eFullscreen:
        monitor = glfwGetPrimaryMonitor();
        break;
    default:
        Assert(false);
        break;
    }

    window = glfwCreateWindow(extent.width, extent.height, Config::kEngineName, monitor, nullptr);

    glfwSetWindowUserPointer(window, this);

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

    return vk::Extent2D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
}

void Window::SetResizeCallback(ResizeCallback resizeCallback_)
{
    resizeCallback = resizeCallback_;

    const auto framebufferSizeCallback = [](GLFWwindow *window, int width, int height)
        {
            const vk::Extent2D extent(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
            const Window *pointer = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
            pointer->resizeCallback(extent);
        };

    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
}

bool Window::ShouldClose() const
{
    return glfwWindowShouldClose(window);
}

void Window::PollEvents() const
{
    glfwPollEvents();
}

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
    Assert(window != nullptr);

    glfwSetWindowUserPointer(window, this);

    // Assert(glfwRawMouseMotionSupported());
    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    // glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
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

    const auto callback = [](GLFWwindow *window, int width, int height)
        {
            const Window *userPointer = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
            const vk::Extent2D extent(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
            userPointer->resizeCallback(extent);
        };

    glfwSetFramebufferSizeCallback(window, callback);
}

void Window::SetKeyInputCallback(KeyInputCallback keyInputCallback_)
{
    keyInputCallback = keyInputCallback_;

    const auto callback = [](GLFWwindow *window, int key, int, int action, int mods)
        {
            const Window *userPointer = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
            userPointer->keyInputCallback(static_cast<Key>(key), static_cast<KeyAction>(action),
                    ModifierFlags(static_cast<uint32_t>(mods)));
        };

    glfwSetKeyCallback(window, callback);
}

void Window::SetMouseInputCallback(MouseInputCallback mouseInputCallback_)
{
    mouseInputCallback = mouseInputCallback_;

    const auto callback = [](GLFWwindow *window, int button, int action, int mods)
        {
            const Window *userPointer = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
            const ModifierFlags modifiers(static_cast<uint32_t>(mods));
            userPointer->mouseInputCallback(static_cast<MouseButton>(button),
                    static_cast<MouseButtonAction>(action), modifiers);
        };

    glfwSetMouseButtonCallback(window, callback);
}

void Window::SetMouseMoveCallback(MouseMoveCallback mouseMoveCallback_)
{
    mouseMoveCallback = mouseMoveCallback_;

    const auto callback = [](GLFWwindow *window, double xPos, double yPos)
        {
            const Window *userPointer = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
            const glm::vec2 position(static_cast<float>(xPos), static_cast<float>(yPos));
            userPointer->mouseMoveCallback(position);
        };

    glfwSetCursorPosCallback(window, callback);
}


bool Window::ShouldClose() const
{
    return glfwWindowShouldClose(window);
}

void Window::PollEvents() const
{
    glfwPollEvents();
}

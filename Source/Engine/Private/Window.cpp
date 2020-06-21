#include "Engine/Window.hpp"

#include "Engine/Config.hpp"
#include "Engine/Engine.hpp"

#include "Utils/Assert.hpp"

namespace SWindow
{
    void SetResizeCallback(GLFWwindow *window)
    {
        const auto callback = [](GLFWwindow *, int32_t width, int32_t height)
            {
                const vk::Extent2D extent(static_cast<uint32_t>(width), static_cast<uint32_t>(height));

                Engine::TriggerEvent(EventType::eResize, extent);
            };

        glfwSetFramebufferSizeCallback(window, callback);
    }

    void SetKeyInputCallback(GLFWwindow *window)
    {
        const auto callback = [](GLFWwindow *, int32_t key, int32_t, int32_t action, int32_t mods)
            {
                const KeyInput keyInput{
                    static_cast<Key>(key),
                    static_cast<KeyAction>(action),
                    ModifierFlags(static_cast<uint32_t>(mods))
                };

                Engine::TriggerEvent(EventType::eKeyInput, keyInput);
            };

        glfwSetKeyCallback(window, callback);
    }

    void SetMouseInputCallback(GLFWwindow *window)
    {
        const auto callback = [](GLFWwindow *, int32_t button, int32_t action, int32_t mods)
            {
                const MouseInput mouseInput{
                    static_cast<MouseButton>(button),
                    static_cast<MouseButtonAction>(action),
                    ModifierFlags(static_cast<uint32_t>(mods))
                };

                Engine::TriggerEvent(EventType::eMouseInput, mouseInput);
            };

        glfwSetMouseButtonCallback(window, callback);
    }

    void SetMouseMoveCallback(GLFWwindow *window)
    {
        const auto callback = [](GLFWwindow *, double xPos, double yPos)
            {
                const glm::vec2 position(static_cast<float>(xPos), static_cast<float>(yPos));
                Engine::TriggerEvent(EventType::eMouseMove, position);
            };

        glfwSetCursorPosCallback(window, callback);
    }
}

Window::Window(const vk::Extent2D &extent, Mode mode)
{
    glfwSetErrorCallback([](int32_t code, const char *description)
        {
            std::cout << "[GLFW] Error " << code << " occured: " << description << std::endl;
        });

    Assert(glfwInit());

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    GLFWmonitor *monitor = nullptr;
    switch (mode)
    {
    case Mode::eWindowed:
        break;
    case Mode::eBorderless:
        glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        break;
    case Mode::eFullscreen:
        monitor = glfwGetPrimaryMonitor();
        break;
    default:
        Assert(false);
        break;
    }

    window = glfwCreateWindow(extent.width, extent.height, Config::kEngineName, monitor, nullptr);
    Assert(window != nullptr);

    Assert(glfwRawMouseMotionSupported());
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    SWindow::SetResizeCallback(window);
    SWindow::SetKeyInputCallback(window);
    SWindow::SetMouseInputCallback(window);
    SWindow::SetMouseMoveCallback(window);
}

Window::~Window()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

vk::Extent2D Window::GetExtent() const
{
    int32_t width, height;

    glfwGetFramebufferSize(window, &width, &height);

    return vk::Extent2D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
}


bool Window::ShouldClose() const
{
    return glfwWindowShouldClose(window);
}

void Window::PollEvents() const
{
    glfwPollEvents();
}

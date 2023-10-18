#include <GLFW/glfw3.h>

#include "Engine/Window.hpp"

#include "Engine/Config.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Filesystem/ImageLoader.hpp"

#include "Utils/Assert.hpp"

namespace Details
{
    static void SetResizeCallback(GLFWwindow* window)
    {
        const auto callback = [](GLFWwindow*, int32_t width, int32_t height)
            {
                const vk::Extent2D extent(static_cast<uint32_t>(width), static_cast<uint32_t>(height));

                Engine::TriggerEvent(EventType::eResize, extent);
            };

        glfwSetFramebufferSizeCallback(window, callback);
    }

    static void SetKeyInputCallback(GLFWwindow* window)
    {
        const auto callback = [](GLFWwindow*, int32_t key, int32_t, int32_t action, int32_t mods)
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

    static void SetMouseInputCallback(GLFWwindow* window)
    {
        const auto callback = [](GLFWwindow*, int32_t button, int32_t action, int32_t mods)
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

    static void SetMouseMoveCallback(GLFWwindow* window)
    {
        const auto callback = [](GLFWwindow*, double xPos, double yPos)
            {
                const glm::vec2 position(static_cast<float>(xPos), static_cast<float>(yPos));
                Engine::TriggerEvent(EventType::eMouseMove, position);
            };

        glfwSetCursorPosCallback(window, callback);
    }

    static GLFWimage LoadIconImage(const Filepath& iconFilepath)
    {
        const ImageSource image = ImageLoader::LoadImage(iconFilepath, 4);

        return GLFWimage{
            static_cast<int32_t>(image.extent.width),
            static_cast<int32_t>(image.extent.height),
            image.data.data
        };
    }

    static void SetWindowIcon(GLFWwindow* window, const std::vector<Filepath>& iconFilepaths)
    {
        std::vector<GLFWimage> iconImages;
        iconImages.reserve(iconFilepaths.size());

        for (const auto& iconFilepath : iconFilepaths)
        {
            iconImages.push_back(LoadIconImage(iconFilepath));
        }

        glfwSetWindowIcon(window, static_cast<int32_t>(iconImages.size()), iconImages.data());

        for (const auto& iconImage : iconImages)
        {
            ImageLoader::FreeImage(iconImage.pixels);
        }
    }
}

Window::Window(const vk::Extent2D& extent, Mode mode)
{
    EASY_FUNCTION()

    glfwSetErrorCallback([](int32_t code, const char* description)
        {
            std::cout << "[GLFW] Error " << code << " occured: " << description << std::endl;
        });

    Assert(glfwInit());

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#ifdef __linux__
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // glfw fails to send signal about resize on Ubuntu 23.04 so we get OutOfDateSwapchain vkError
#else
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
#endif

    GLFWmonitor* monitor = nullptr;
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

    const int32_t width = static_cast<int32_t>(extent.width);
    const int32_t height = static_cast<int32_t>(extent.height);

    window = glfwCreateWindow(width, height, Config::kEngineName, monitor, nullptr);
    Assert(window);

    Details::SetResizeCallback(window);
    Details::SetKeyInputCallback(window);
    Details::SetMouseInputCallback(window);
    Details::SetMouseMoveCallback(window);
    Details::SetWindowIcon(window, Config::kEngineLogos);
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

void Window::SetCursorMode(CursorMode mode)
{
    cursorMode = mode;

    switch (cursorMode)
    {
    case CursorMode::eEnabled:
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        break;
    case CursorMode::eHidden:
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        break;
    case CursorMode::eDisabled:
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        break;
    default:
        Assert(false);
        break;
    }

    if (cursorMode == CursorMode::eDisabled)
    {
        Assert(glfwRawMouseMotionSupported());
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
    else
    {
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
    }
}

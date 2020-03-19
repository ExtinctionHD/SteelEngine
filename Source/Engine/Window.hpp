#pragma once

#include "InputHelpers.hpp"

enum class WindowMode
{
    eWindowed,
    eBorderless,
    eFullscreen
};

using ResizeCallback = std::function<void(const vk::Extent2D &)>;
using KeyInputCallback = std::function<void(Key, KeyAction, ModifierFlags)>;
using MouseInputCallback = std::function<void(MouseButton, MouseButtonAction, ModifierFlags)>;
using MouseMoveCallback = std::function<void(const glm::vec2&)>;

class Window
{
public:
    Window(const vk::Extent2D &extent, WindowMode mode);
    ~Window();

    GLFWwindow *Get() const { return window; }

    vk::Extent2D GetExtent() const;

    void SetResizeCallback(ResizeCallback resizeCallback_);

    void SetKeyInputCallback(KeyInputCallback keyInputCallback_);

    void SetMouseInputCallback(MouseInputCallback mouseInputCallback_);

    void SetMouseMoveCallback(MouseMoveCallback mouseMoveCallback_);

    bool ShouldClose() const;

    void PollEvents() const;

private:
    GLFWwindow *window;

    ResizeCallback resizeCallback;
    KeyInputCallback keyInputCallback;
    MouseInputCallback mouseInputCallback;
    MouseMoveCallback mouseMoveCallback;
};

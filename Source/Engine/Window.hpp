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

    enum class CursorMode
    {
        eEnabled,
        eHidden,
        eDisabled
    };

    Window(const vk::Extent2D& extent, Mode mode);
    ~Window();

    static Mode ParseWindowMode(const std::string& mode);

    GLFWwindow* Get() const { return window; }

    vk::Extent2D GetExtent() const;

    bool ShouldClose() const;

    void PollEvents() const;

    CursorMode GetCursorMode() const { return cursorMode; }
    void SetCursorMode(CursorMode mode);

private:
    GLFWwindow* window;

    CursorMode cursorMode = CursorMode::eEnabled;
};

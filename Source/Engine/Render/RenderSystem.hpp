#pragma once

struct GLFWwindow;

class RenderSystem
{
public:
    RenderSystem() = default;

    void ConnectWindow(GLFWwindow* window);

    void Process() const;

private:
};

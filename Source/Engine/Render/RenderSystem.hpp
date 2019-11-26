#pragma once

struct GLFWwindow;

class RenderSystem
{
public:
    RenderSystem();

    void ConnectWindow(GLFWwindow* window);

    void Process() const;

private:
};

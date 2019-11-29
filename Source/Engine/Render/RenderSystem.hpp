#pragma once

struct GLFWwindow;

class RenderSystem
{
public:
    RenderSystem();

    void SetupWindow(GLFWwindow* window);

    void Process() const;

private:
};

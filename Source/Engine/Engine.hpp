#pragma once

#include "Engine/Window.hpp"
#include "Engine/Render/RenderSystem.hpp"

class Engine
{
public:
    Engine();

    void Run() const;

private:
    std::unique_ptr<Window> window;

    std::unique_ptr<RenderSystem> renderSystem;
};

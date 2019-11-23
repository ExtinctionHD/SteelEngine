#pragma once

#include "Engine/Window.hpp"
#include "Render/RenderSystem.hpp"

#include <memory>

class Engine
{
public:
    Engine();

    void Run() const;

private:
    void ProcessSystems() const;

    std::unique_ptr<Window> window;

    std::unique_ptr<RenderSystem> renderSystem;
};

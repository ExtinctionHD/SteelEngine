#pragma once
#include <memory>

#include "Engine/Window.hpp"
#include "Render/RenderSystem.hpp"

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

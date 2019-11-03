#pragma once

#include "Engine/Window.hpp"

#include <memory>

class Engine
{
public:
    static Engine *Instance();

    void Run();

private:
    inline static Engine *instance = nullptr;

    Engine() = default;

    std::unique_ptr<Window> window;
};

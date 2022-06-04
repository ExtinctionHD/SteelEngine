#pragma once
#include "Scene2.hpp"

class Window;
class FrameLoop;

namespace Steel
{
    class Engine2
    {
    public:
        static void Create();
        static void Run();
        static void Destroy();

    private:
        static std::unique_ptr<Window> window;

        static Scene2 scene;
    };
}
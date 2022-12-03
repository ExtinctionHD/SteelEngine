#include "Engine/Engine.hpp"

int main(int, char**)
{
    EASY_PROFILER_ENABLE
    profiler::startListen();

    Engine::Create();
    Engine::Run();
    Engine::Destroy();

    return 0;
}

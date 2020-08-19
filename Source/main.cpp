#include "Engine/Engine.hpp"

int main(int, char**)
{
    Engine::Create();

    Engine::Run();

    Engine::Destroy();

    return 0;
}

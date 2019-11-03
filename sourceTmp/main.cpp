#include "Engine/Engine.hpp"

#include <iostream>

int main(int, char **)
{
    std::cout << "Hello world!" << std::endl;

    Engine *engine = Engine::Instance();
    engine->Run();

    return 0;
}

#include "Engine/Engine.hpp"

#include <iostream>

int main(int, char **)
{
    const auto engine = std::make_unique<Engine>();
    engine->Run();

    return 0;
}

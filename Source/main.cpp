#include "Engine/Engine.hpp"

int main(int, char **)
{
    const auto engine = std::make_unique<Engine>();
    engine->Run();

    return 0;
}

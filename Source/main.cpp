#include "Engine/Engine.hpp"
#include "Engine2/Engine2.hpp"

int main(int, char**)
{
    //Engine::Create();
    //Engine::Run();
    //Engine::Destroy();

    Steel::Engine2::Create();
    Steel::Engine2::Run();
    Steel::Engine2::Destroy();

    return 0;
}

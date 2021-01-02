#include "Engine/Render/Renderer.hpp"

std::unique_ptr<DirectLighting> Renderer::directLighting;

void Renderer::Create()
{
    directLighting = std::make_unique<DirectLighting>();
}

void Renderer::Destroy()
{
    directLighting.reset();
}

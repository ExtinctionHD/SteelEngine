#pragma once

#include "Engine/Scene/DirectLighting.hpp"

class Renderer
{
public:
    static void Create();
    static void Destroy();

    static std::unique_ptr<DirectLighting> directLighting;

    static vk::Sampler defaultSampler;

    static Texture blackTexture;
    static Texture whiteTexture;
    static Texture normalTexture;
};

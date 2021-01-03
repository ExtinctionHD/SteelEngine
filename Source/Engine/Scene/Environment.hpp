#pragma once
#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"
#include "Shaders/Common/Common.h"

class Filepath;
class DirectLightRetriever;

class Environment
{
public:
    Environment(const Filepath& path);
    ~Environment();

    const Texture& GetTexture() const { return texture; }

    const DirectLight& GetDirectLight() const { return directLight; }

private:
    Texture texture;

    DirectLight directLight;
};

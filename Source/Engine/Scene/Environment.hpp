#pragma once
#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"

class Filepath;

class Environment
{
public:
    Environment(const Filepath& path);
    ~Environment();

    const Texture& GetTexture() const { return texture; }

private:
    Texture texture;

    glm::vec4 directLight;
};

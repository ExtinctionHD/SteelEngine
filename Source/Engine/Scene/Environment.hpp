#pragma once
#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"

class Filepath;
class DirectLightRetriever;

class Environment
{
public:
    Environment(const Filepath& path);
    ~Environment();

    const Texture& GetTexture() const { return texture; }

    const glm::vec3& GetLightDirection() const { return lightDirection; }

private:
    Texture texture;

    glm::vec3 lightDirection;
};

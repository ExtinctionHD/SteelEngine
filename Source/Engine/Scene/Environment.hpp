#pragma once
#include "Engine/Scene/ImageBasedLighting.hpp"
#include "Engine/Render/Vulkan/Resources/TextureHelpers.hpp"
#include "Shaders/Common/Common.h"

class Filepath;

class Environment
{
public:
    Environment(const Filepath& path);
    ~Environment();

    const Texture& GetTexture() const { return texture; }

    const DirectLight& GetDirectLight() const { return directLight; }

    const Texture& GetIrradianceTexture() const { return iblTextures.irradiance; }

    const Texture& GetReflectionTexture() const { return iblTextures.reflection; }

private:
    Texture texture;

    DirectLight directLight;

    ImageBasedLighting::Textures iblTextures;
};

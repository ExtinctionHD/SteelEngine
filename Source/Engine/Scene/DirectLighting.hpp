#pragma once

struct Texture;

namespace DirectLighting
{
    glm::vec3 RetrieveLightDirection(const Texture& panoramaTexture);
};

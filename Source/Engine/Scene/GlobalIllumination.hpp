#pragma once

class Scene;
class Environment;
struct AABBox;

struct SphericalHarmonicsGrid
{
    std::vector<glm::vec3> positions;
};

class GlobalIllumination
{
public:
    GlobalIllumination() = default;
    ~GlobalIllumination() = default;

    SphericalHarmonicsGrid Generate(Scene* scene, Environment* environment, const AABBox& bbox);
};

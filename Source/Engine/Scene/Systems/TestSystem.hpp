#pragma once

#include "Engine/Scene/Systems/System.hpp"

class Scene;

class TestSystem
    : public System
{
public:
    void Process(Scene& scene, float deltaSeconds) override;

private:
    std::unique_ptr<Scene> helmetScene;
};

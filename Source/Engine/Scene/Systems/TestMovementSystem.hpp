#pragma once

#include "Engine/Scene/Systems/System.hpp"

class Scene;

class TestMovementSystem
        : public System
{
public:
    TestMovementSystem();

    void Process(Scene& scene, float deltaSeconds) override;

private:
};

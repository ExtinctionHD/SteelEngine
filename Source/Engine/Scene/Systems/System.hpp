#pragma once

class Scene;

class System
{
public:
    System() = default;

    virtual ~System() = default;

    virtual void Process(Scene& scene, float deltaSeconds);
};

inline void System::Process(Scene&, float)
{
}

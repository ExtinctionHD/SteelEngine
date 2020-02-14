#pragma once

class System
{
public:
    System() = default;

    virtual ~System() = default;

    virtual void Process(float timeElapsed) = 0;

    virtual void OnResize(const vk::Extent2D &extent);
};

inline void System::OnResize(const vk::Extent2D &) {}

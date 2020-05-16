#pragma once

class System
{
public:
    System() = default;

    virtual ~System() = default;

    virtual void Process(float deltaSeconds);
};

inline void System::Process(float) {}

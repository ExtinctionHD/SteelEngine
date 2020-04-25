#pragma once

#include "Engine/InputHelpers.hpp"

struct EngineState;

class System
{
public:
    System() = default;

    virtual ~System() = default;

    virtual void Process(float deltaSeconds, EngineState &engineState);

    virtual void OnResize(const vk::Extent2D &extent);

    virtual void OnKeyInput(Key key, KeyAction action, ModifierFlags modifiers);

    virtual void OnMouseInput(MouseButton button, MouseButtonAction action, ModifierFlags modifiers);

    virtual void OnMouseMove(const glm::vec2 &position);
};

inline void System::Process(float, EngineState &) {}
inline void System::OnResize(const vk::Extent2D &) {}
inline void System::OnKeyInput(Key, KeyAction, ModifierFlags) {}
inline void System::OnMouseInput(MouseButton, MouseButtonAction, ModifierFlags) {}
inline void System::OnMouseMove(const glm::vec2 &) {}

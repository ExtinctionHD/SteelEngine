#pragma once

#include "Engine/InputHelpers.hpp"

#include "Utils/Helpers.hpp"

enum class EventType
{
    eResize,
    eKeyInput,
    eMouseInput,
    eMouseMove,
    eCameraUpdate,
};

using EventHandler = std::function<void(std::any)>;

struct KeyInput
{
    Key key;
    KeyAction action;
    ModifierFlags modifiers;
};

struct MouseInput
{
    MouseButton button;
    MouseButtonAction action;
    ModifierFlags modifiers;
};

namespace Direction
{
    const glm::vec3 kForward = -Vector3::kZ;
    const glm::vec3 kBackward = Vector3::kZ;
    const glm::vec3 kRight = Vector3::kX;
    const glm::vec3 kLeft = -Vector3::kX;
    const glm::vec3 kUp = Vector3::kY;
    const glm::vec3 kDown = -Vector3::kY;
}

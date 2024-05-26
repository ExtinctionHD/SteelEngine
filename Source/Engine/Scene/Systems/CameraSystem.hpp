#pragma once

#include "Engine/InputHelpers.hpp"
#include "Engine/Scene/Systems/System.hpp"
#include "Engine/Scene/Components/CameraComponent.hpp"

class Scene;
struct KeyInput;

class CameraSystem
        : public System
{
public:
    struct Parameters
    {
        float sensitivity;
        float baseSpeed;
        float speedMultiplier;
    };

    enum class MovementAxis
    {
        eForward,
        eLeft,
        eUp,
    };

    using MovementKeyBindings = std::map<MovementAxis, std::pair<Key, Key>>;
    using SpeedKeyBindings = std::vector<Key>;

    CameraSystem();

    void Process(Scene& scene, float deltaSeconds) override;

private:
    enum class MovementValue
    {
        ePositive,
        eWeakPositive,
        eNone,
        eWeakNegative,
        eNegative
    };

    struct MovementState
    {
        uint32_t enabled : 1 = false;
        uint32_t moving : 1 = false;

        std::map<MovementAxis, MovementValue> movement{
            { MovementAxis::eForward, MovementValue::eNone },
            { MovementAxis::eLeft, MovementValue::eNone },
            { MovementAxis::eUp, MovementValue::eNone },
        };

        float speedIndex = 0.0f;
    };

    struct RotationState
    {
        uint32_t enabled : 1 = false;
        uint32_t rotated : 1 = false;
        glm::vec2 yawPitch{};
    };

    struct ResizeState
    {
        bool resized = false;
        float width = 0.0f;
        float height = 0.0f;
    };

    Parameters parameters;
    MovementKeyBindings movementKeyBindings;
    SpeedKeyBindings speedKeyBindings;

    ResizeState resizeState;
    RotationState rotationState;
    MovementState movementState;

    std::optional<glm::vec2> lastMousePosition;

    void HandleResizeEvent(const vk::Extent2D& extent);
    void HandleKeyInputEvent(const KeyInput& keyInput);
    void HandleMouseMoveEvent(const glm::vec2& position);
    void HandleMouseInputEvent(const MouseInput& mouseInput);

    bool IsCameraMoving() const;

    glm::vec3 GetMovementDirection() const;
};

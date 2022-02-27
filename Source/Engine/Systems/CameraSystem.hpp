#pragma once

#include "Engine/Systems/System.hpp"
#include "Engine/InputHelpers.hpp"
#include "Engine/Camera.hpp"

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

    CameraSystem(Camera* camera_);

    void Process(float deltaSeconds) override;

private:
    enum class MovementValue
    {
        ePositive,
        eWeakPositive,
        eNone,
        eWeakNegative,
        eNegative
    };

    struct CameraState
    {
        glm::vec2 yawPitch = glm::vec2(0.0f, 0.0f);

        std::map<MovementAxis, MovementValue> movement{
            { MovementAxis::eForward, MovementValue::eNone },
            { MovementAxis::eLeft, MovementValue::eNone },
            { MovementAxis::eUp, MovementValue::eNone },
        };

        float speedIndex = 0.0f;
        bool rotationEnabled = false;
    };

    Camera* camera;

    Parameters parameters;
    MovementKeyBindings movementKeyBindings;
    SpeedKeyBindings speedKeyBindings;

    CameraState state;

    std::optional<glm::vec2> lastMousePosition;

    void HandleResizeEvent(const vk::Extent2D& extent) const;
    void HandleKeyInputEvent(const KeyInput& keyInput);
    void HandleMouseMoveEvent(const glm::vec2& position);
    void HandleMouseInputEvent(const MouseInput& mouseInput);

    bool IsCameraMoved() const;

    glm::vec3 GetMovementDirection() const;
};

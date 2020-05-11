#pragma once

#include "Engine/InputHelpers.hpp"
#include "Engine/System.hpp"
#include "Engine/Camera.hpp"

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

    CameraSystem(Camera *camera_, const Parameters &parameters_,
            const MovementKeyBindings &movementKeyBindings_,
            const SpeedKeyBindings &speedKeyBindings_);

    virtual ~CameraSystem() = default;

    void Process(float deltaSeconds, EngineState &engineState) override;

    void OnResize(const vk::Extent2D &extent) override;

    void OnKeyInput(Key key, KeyAction action, ModifierFlags modifiers) override;

    void OnMouseMove(const glm::vec2 &position) override;

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
        bool rotated = false;
    };

    Camera *camera;
    Parameters parameters;
    MovementKeyBindings movementKeyBindings;
    SpeedKeyBindings speedKeyBindings;
    CameraState state;

    std::optional<glm::vec2> lastMousePosition;

    void OnKeyPress(Key key);
    void OnKeyRelease(Key key);

    bool CameraMoved() const;

    glm::vec3 GetMovementDirection() const;
};

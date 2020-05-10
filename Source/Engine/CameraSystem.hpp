#pragma once

#include "Engine/InputHelpers.hpp"
#include "Engine/System.hpp"
#include "Engine/Camera.hpp"

struct CameraParameters
{
    float sensitivity;
    float baseSpeed;
    float speedMultiplier;
};

enum class CameraMovementAxis
{
    eForward,
    eLeft,
    eUp,
};

using CameraMovementKeyBindings = std::map<CameraMovementAxis, std::pair<Key, Key>>;
using CameraSpeedKeyBindings = std::vector<Key>;

class CameraSystem
        : public System
{
public:
    CameraSystem(Camera &camera_, const CameraParameters &parameters_,
            const CameraMovementKeyBindings &movementKeyBindings_,
            const CameraSpeedKeyBindings &speedKeyBindings_);

    virtual ~CameraSystem() = default;

    void Process(float deltaSeconds, EngineState &engineState) override;

    void OnResize(const vk::Extent2D &extent) override;

    void OnKeyInput(Key key, KeyAction action, ModifierFlags modifiers) override;

    void OnMouseMove(const glm::vec2 &position) override;

private:
    enum class MovementValue
    {
        ePositive,
        eNone,
        eNegative
    };

    struct CameraState
    {
        glm::vec2 yawPitch = glm::vec2(0.0f, 0.0f);

        std::map<CameraMovementAxis, MovementValue> movement{
            { CameraMovementAxis::eForward, MovementValue::eNone },
            { CameraMovementAxis::eLeft, MovementValue::eNone },
            { CameraMovementAxis::eUp, MovementValue::eNone },
        };

        float speedIndex = 0.0f;
        bool rotated = false;
    };

    Camera &camera;

    CameraParameters parameters;

    CameraMovementKeyBindings movementKeyBindings;
    CameraSpeedKeyBindings speedKeyBindings;

    CameraState state;

    std::optional<glm::vec2> lastMousePosition;


    glm::vec3 GetMovementDirection() const;

    bool CameraMoved() const;
};

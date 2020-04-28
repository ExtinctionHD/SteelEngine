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

struct CameraKeyBindings
{
    Key forward;
    Key backward;
    Key left;
    Key right;
    Key up;
    Key down;
    std::vector<Key> speed;
};

class CameraSystem
        : public System
{
public:
    CameraSystem(Camera &camera_,
            const CameraParameters &parameters_,
            const CameraKeyBindings &keyBindings_);

    virtual ~CameraSystem() = default;

    void Process(float deltaSeconds, EngineState &engineState) override;

    void OnResize(const vk::Extent2D &extent) override;

    void OnKeyInput(Key key, KeyAction action, ModifierFlags modifiers) override;

    void OnMouseMove(const glm::vec2 &position) override;

private:
    enum class Movement
    {
        ePositive,
        eNone,
        eNegative
    };

    struct CameraState
    {
        glm::vec2 yawPitch = glm::vec2(0.0f, 0.0f);
        Movement forwardMovement = Movement::eNone;
        Movement leftMovement = Movement::eNone;
        Movement upMovement = Movement::eNone;
        float speedIndex = 0.0f;
        bool rotated = false;
    };

    Camera &camera;

    CameraParameters parameters;

    CameraKeyBindings keyBindings;

    CameraState state;

    std::optional<glm::vec2> lastMousePosition;


    glm::vec3 GetMovementDirection() const;

    bool CameraMoved() const;
};

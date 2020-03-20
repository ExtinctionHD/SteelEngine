#pragma once

#include "Engine/InputHelpers.hpp"
#include "Engine/System.hpp"
#include "Engine/Camera.hpp"

struct CameraParameters
{
    float sensitivity;
    float baseSpeed;
    float boostedSpeed;
};

struct CameraKeyBindings
{
    Key forward;
    Key backward;
    Key left;
    Key right;
    Key up;
    Key down;
    Key speedUp;
};

class CameraSystem
        : public System
{
public:
    CameraSystem(std::shared_ptr<Camera> camera_, const CameraParameters &parameters_,
            const CameraKeyBindings &keyBindings_);

    virtual ~CameraSystem() = default;

    void Process(float timeElapsed) override;

    void OnResize(const vk::Extent2D &extent) override;

    void OnKeyInput(Key key, KeyAction action, ModifierFlags modifiers) override;

    void OnMouseInput(MouseButton button, MouseButtonAction action, ModifierFlags modifiers) override;

    void OnMouseMove(const glm::vec2 &position) override;

private:
    std::shared_ptr<Camera> camera;

    CameraParameters parameters;

    CameraKeyBindings keyBindings;

    std::optional<glm::vec2> lastMousePosition;

    glm::vec2 yawPitch = glm::vec2(0.0f, 0.0f);
};

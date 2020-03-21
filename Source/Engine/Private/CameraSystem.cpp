#include "Engine/CameraSystem.hpp"

#include "Engine/EngineHelpers.hpp"

namespace SCameraSystem
{
    constexpr float kSensitivityReduction = 0.001f;
    constexpr float kPitchLimitRad = glm::radians(89.0f);

    glm::quat GetOrientationQuat(const glm::vec2 yawPitch)
    {
        const glm::quat yawQuat = glm::angleAxis(yawPitch.x, Direction::kDown);
        const glm::quat pitchQuat = glm::angleAxis(yawPitch.y, Direction::kRight);

        return glm::normalize(yawQuat * pitchQuat);
    }
}

CameraSystem::CameraSystem(std::shared_ptr<Camera> camera_, const CameraParameters &parameters_,
        const CameraKeyBindings &keyBindings_)
    : camera(camera_)
    , parameters(parameters_)
    , keyBindings(keyBindings_)
{}

void CameraSystem::Process(float deltaSeconds)
{
    const glm::vec3 movementDirection = SCameraSystem::GetOrientationQuat(yawPitch) * GetMovementDirection();

    const float speed = state.speedUp ? parameters.boostedSpeed : parameters.baseSpeed;
    const float distance = speed * deltaSeconds;

    camera->AccessData().position += glm::vec4(movementDirection * distance, 1.0f);

    camera->UpdateView();
}

void CameraSystem::OnResize(const vk::Extent2D &extent)
{
    camera->SetAspect(extent.width / static_cast<float>(extent.height));
}

void CameraSystem::OnKeyInput(Key key, KeyAction action, ModifierFlags)
{
    switch (action)
    {
    case KeyAction::ePress:
    case KeyAction::eRepeat:
        if (key == keyBindings.forward && state.forwardMovement == Movement::eNone)
        {
            state.forwardMovement = Movement::ePositive;
        }
        else if (key == keyBindings.backward && state.forwardMovement == Movement::eNone)
        {
            state.forwardMovement = Movement::eNegative;
        }
        else if (key == keyBindings.left && state.leftMovement == Movement::eNone)
        {
            state.leftMovement = Movement::ePositive;
        }
        else if (key == keyBindings.right && state.leftMovement == Movement::eNone)
        {
            state.leftMovement = Movement::eNegative;
        }
        else if (key == keyBindings.up && state.upMovement == Movement::eNone)
        {
            state.upMovement = Movement::ePositive;
        }
        else if (key == keyBindings.down && state.upMovement == Movement::eNone)
        {
            state.upMovement = Movement::eNegative;
        }
        else if (key == keyBindings.speedUp)
        {
            state.speedUp = true;
        }
        break;

    case KeyAction::eRelease:
        if (key == keyBindings.forward)
        {
            state.forwardMovement = Movement::eNone;
        }
        else if (key == keyBindings.backward)
        {
            state.forwardMovement = Movement::eNone;
        }
        else if (key == keyBindings.left)
        {
            state.leftMovement = Movement::eNone;
        }
        else if (key == keyBindings.right)
        {
            state.leftMovement = Movement::eNone;
        }
        else if (key == keyBindings.up)
        {
            state.upMovement = Movement::eNone;
        }
        else if (key == keyBindings.down)
        {
            state.upMovement = Movement::eNone;
        }
        else if (key == keyBindings.speedUp)
        {
            state.speedUp = false;
        }
        break;
    }
}

void CameraSystem::OnMouseMove(const glm::vec2 &position)
{
    if (lastMousePosition.has_value())
    {
        glm::vec2 delta = position - lastMousePosition.value();
        delta.y = -delta.y;

        yawPitch += delta * parameters.sensitivity * SCameraSystem::kSensitivityReduction;
        yawPitch.y = std::clamp(yawPitch.y, -SCameraSystem::kPitchLimitRad, SCameraSystem::kPitchLimitRad);

        const glm::vec3 direction = SCameraSystem::GetOrientationQuat(yawPitch) * Direction::kForward;
        camera->AccessData().direction = glm::vec4(glm::normalize(direction), 0.0f);
    }

    lastMousePosition = position;
}

glm::vec3 CameraSystem::GetMovementDirection() const
{
    glm::vec3 movementDirection = Vector3::kZero;

    switch (state.forwardMovement)
    {
    case Movement::ePositive:
        movementDirection += Direction::kForward;
        break;
    case Movement::eNone:
        break;
    case Movement::eNegative:
        movementDirection += Direction::kBackward;
        break;
    }

    switch (state.leftMovement)
    {
    case Movement::ePositive:
        movementDirection += Direction::kLeft;
        break;
    case Movement::eNone:
        break;
    case Movement::eNegative:
        movementDirection += Direction::kRight;
        break;
    }

    switch (state.upMovement)
    {
    case Movement::ePositive:
        movementDirection += Direction::kUp;
        break;
    case Movement::eNone:
        break;
    case Movement::eNegative:
        movementDirection += Direction::kDown;
        break;
    }

    if (movementDirection != Vector3::kZero)
    {
        movementDirection = glm::normalize(movementDirection);
    }

    return movementDirection;
}

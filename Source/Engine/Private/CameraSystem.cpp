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

CameraSystem::CameraSystem(Camera &camera_,
        const CameraParameters &parameters_,
        const CameraKeyBindings &keyBindings_)
    : camera(camera_)
    , parameters(parameters_)
    , keyBindings(keyBindings_)
{}

void CameraSystem::Process(float deltaSeconds, EngineState &engineState)
{
    engineState.cameraUpdated = state.rotated || CameraMoved();

    const glm::vec3 movementDirection = SCameraSystem::GetOrientationQuat(state.yawPitch) * GetMovementDirection();

    const float speed = parameters.baseSpeed * std::powf(parameters.speedMultiply, state.speedIndex);
    const float distance = speed * deltaSeconds;

    camera.SetPosition(camera.GetDescription().position + movementDirection * distance);

    state.rotated = false;
}

void CameraSystem::OnResize(const vk::Extent2D &extent)
{
    if (extent.width != 0 && extent.height != 0)
    {
        camera.SetAspect(extent.width / static_cast<float>(extent.height));
    }
}

void CameraSystem::OnKeyInput(Key key, KeyAction action, ModifierFlags)
{
    switch (action)
    {
    case KeyAction::ePress:
    {
        const auto it = std::find(keyBindings.speed.begin(), keyBindings.speed.end(), key);
        if (it != keyBindings.speed.end())
        {
            state.speedIndex = static_cast<float>(std::distance(keyBindings.speed.begin(), it));
        }
    }
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
        break;
    }
}

void CameraSystem::OnMouseMove(const glm::vec2 &position)
{
    if (lastMousePosition.has_value())
    {
        glm::vec2 delta = position - lastMousePosition.value();
        delta.y = -delta.y;

        state.yawPitch += delta * parameters.sensitivity * SCameraSystem::kSensitivityReduction;
        state.yawPitch.y = std::clamp(state.yawPitch.y, -SCameraSystem::kPitchLimitRad, SCameraSystem::kPitchLimitRad);

        const glm::vec3 direction = SCameraSystem::GetOrientationQuat(state.yawPitch) * Direction::kForward;
        camera.SetDirection(glm::normalize(direction));
    }

    lastMousePosition = position;

    state.rotated = true;
}

glm::vec3 CameraSystem::GetMovementDirection() const
{
    glm::vec3 movementDirection(0.0f);

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

    if (movementDirection != glm::vec3(0.0f))
    {
        movementDirection = glm::normalize(movementDirection);
    }

    return movementDirection;
}

bool CameraSystem::CameraMoved() const
{
    return state.forwardMovement != Movement::eNone
            || state.leftMovement != Movement::eNone
            || state.upMovement != Movement::eNone;
}

#include "Engine/Systems/CameraSystem.hpp"

#include "Engine/Engine.hpp"
#include "Engine/Config.hpp"

namespace Details
{
    static constexpr float kSensitivityFactor = 0.001f;
    static constexpr float kPitchLimitRad = glm::radians(89.0f);

    static const std::map<CameraSystem::MovementAxis, glm::vec3> kMovementAxisDirections{
        { CameraSystem::MovementAxis::eForward, Direction::kForward },
        { CameraSystem::MovementAxis::eLeft, Direction::kLeft },
        { CameraSystem::MovementAxis::eUp, Direction::kUp },
    };

    static glm::vec2 GetYawPitch(const glm::vec3& direction)
    {
        glm::vec2 yawPitch;

        yawPitch.x = glm::atan(direction.x, -direction.z);
        yawPitch.y = std::atan2(direction.y, glm::length(glm::vec2(direction.x, direction.z)));

        return yawPitch;
    }

    static glm::quat GetOrientationQuat(const glm::vec2 yawPitch)
    {
        const glm::quat yawQuat = glm::angleAxis(yawPitch.x, Direction::kDown);
        const glm::quat pitchQuat = glm::angleAxis(yawPitch.y, Direction::kRight);

        return glm::normalize(yawQuat * pitchQuat);
    }
}

CameraSystem::CameraSystem()
    : parameters(Config::DefaultCamera::kSystemParameters)
    , movementKeyBindings(Config::DefaultCamera::kMovementKeyBindings)
    , speedKeyBindings(Config::DefaultCamera::kSpeedKeyBindings)
{
    Engine::AddEventHandler<vk::Extent2D>(EventType::eResize,
            MakeFunction(this, &CameraSystem::HandleResizeEvent));

    Engine::AddEventHandler<KeyInput>(EventType::eKeyInput,
            MakeFunction(this, &CameraSystem::HandleKeyInputEvent));

    Engine::AddEventHandler<glm::vec2>(EventType::eMouseMove,
            MakeFunction(this, &CameraSystem::HandleMouseMoveEvent));

    Engine::AddEventHandler<MouseInput>(EventType::eMouseInput,
            MakeFunction(this, &CameraSystem::HandleMouseInputEvent));
}

void CameraSystem::Process(Scene& scene, float deltaSeconds)
{
    if constexpr (Config::kStaticCamera)
    {
        return;
    }

    auto& cameraComponent = scene.ctx().get<CameraComponent>();

    if (resizeState.resized)
    {
        cameraComponent.projection.width = resizeState.width;
        cameraComponent.projection.height = resizeState.height;

        cameraComponent.projMatrix = CameraHelpers::CalculateProjMatrix(cameraComponent.projection);
    }

    if (rotationState.rotated)
    {
        const glm::vec2 yawPitch = Details::GetYawPitch(cameraComponent.location.direction);
        const glm::quat orientationQuat = Details::GetOrientationQuat(yawPitch + rotationState.yawPitch);

        cameraComponent.location.direction = orientationQuat * Direction::kForward;

        rotationState.yawPitch = {};
    }

    if (movementState.moving)
    {
        const glm::vec2 yawPitch = Details::GetYawPitch(cameraComponent.location.direction);
        const glm::quat orientationQuat = Details::GetOrientationQuat(yawPitch);
        const glm::vec3 movementDirection = orientationQuat * GetMovementDirection();

        const float speed = parameters.baseSpeed * std::powf(parameters.speedMultiplier, movementState.speedIndex);

        cameraComponent.location.position += movementDirection * speed * deltaSeconds;
    }

    if (rotationState.rotated || movementState.moving)
    {
        cameraComponent.viewMatrix = CameraHelpers::CalculateViewMatrix(cameraComponent.location);

        Engine::TriggerEvent(EventType::eCameraUpdate);
    }

    resizeState.resized = false;
    rotationState.rotated = false;
}

void CameraSystem::HandleResizeEvent(const vk::Extent2D& extent)
{
    if (extent.width != 0 && extent.height != 0)
    {
        resizeState.resized = true;
        resizeState.width = static_cast<float>(extent.width);
        resizeState.height = static_cast<float>(extent.height);
    }
}

void CameraSystem::HandleKeyInputEvent(const KeyInput& keyInput)
{
    const auto& [key, action, modifiers] = keyInput;

    if (action == KeyAction::eRepeat)
    {
        return;
    }

    if (action == KeyAction::ePress)
    {
        const auto it = std::ranges::find(speedKeyBindings, key);
        if (it != speedKeyBindings.end())
        {
            movementState.speedIndex = static_cast<float>(std::distance(speedKeyBindings.begin(), it));

            return;
        }
    }

    const auto pred = [&key](const MovementKeyBindings::value_type& entry)
        {
            return entry.second.first == key || entry.second.second == key;
        };

    const auto it = std::ranges::find_if(movementKeyBindings, pred);

    if (it != movementKeyBindings.end())
    {
        const auto& [axis, keys] = *it;

        MovementValue& value = movementState.movement.at(axis);

        switch (action)
        {
        case KeyAction::ePress:
            if (value == MovementValue::eNone)
            {
                value = key == keys.first ? MovementValue::ePositive : MovementValue::eNegative;
            }
            else
            {
                value = key == keys.first ? MovementValue::eWeakNegative : MovementValue::eWeakPositive;
            }
            break;

        case KeyAction::eRelease:
            if (value == MovementValue::ePositive || value == MovementValue::eNegative)
            {
                value = MovementValue::eNone;
            }
            else
            {
                value = key == keys.first ? MovementValue::eNegative : MovementValue::ePositive;
            }
            break;

        case KeyAction::eRepeat:
            break;
        }

        movementState.moving = IsCameraMoving();
    }
}

void CameraSystem::HandleMouseMoveEvent(const glm::vec2& position)
{
    if constexpr (Config::kStaticCamera)
    {
        return;
    }

    if (rotationState.enabled)
    {
        if (lastMousePosition.has_value())
        {
            glm::vec2 delta = position - lastMousePosition.value();
            delta.y = -delta.y;

            rotationState.rotated = true;

            rotationState.yawPitch += delta * parameters.sensitivity * Details::kSensitivityFactor;
        }

        lastMousePosition = position;
    }
    else
    {
        lastMousePosition = std::nullopt;
    }
}

void CameraSystem::HandleMouseInputEvent(const MouseInput& mouseInput)
{
    if (mouseInput.button == Config::DefaultCamera::kControlMouseButton)
    {
        switch (mouseInput.action)
        {
        case MouseButtonAction::ePress:
            rotationState.enabled = true;
            break;
        case MouseButtonAction::eRelease:
            rotationState.enabled = false;
            break;
        default:
            break;
        }
    }
}

bool CameraSystem::IsCameraMoving() const
{
    constexpr auto pred = [](const auto& entry)
        {
            return entry.second != MovementValue::eNone;
        };

    return std::ranges::any_of(movementState.movement, pred);
}

glm::vec3 CameraSystem::GetMovementDirection() const
{
    glm::vec3 movementDirection(0.0f);

    for (const auto& [axis, value] : movementState.movement)
    {
        switch (value)
        {
        case MovementValue::ePositive:
        case MovementValue::eWeakPositive:
            movementDirection += Details::kMovementAxisDirections.at(axis);
            break;
        case MovementValue::eWeakNegative:
        case MovementValue::eNegative:
            movementDirection -= Details::kMovementAxisDirections.at(axis);
            break;
        case MovementValue::eNone:
            break;
        }
    }

    if (movementDirection != Vector3::kZero)
    {
        movementDirection = glm::normalize(movementDirection);
    }

    return movementDirection;
}

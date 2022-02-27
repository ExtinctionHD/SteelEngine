#include "Engine/Systems/CameraSystem.hpp"

#include "Engine/Engine.hpp"
#include "Engine/Config.hpp"

namespace Details
{
    static constexpr float kSensitivityReduction = 0.001f;
    static constexpr float kPitchLimitRad = glm::radians(89.0f);

    static const std::map<CameraSystem::MovementAxis, glm::vec3> kMovementAxisDirections{
        { CameraSystem::MovementAxis::eForward, Direction::kForward },
        { CameraSystem::MovementAxis::eLeft, Direction::kLeft },
        { CameraSystem::MovementAxis::eUp, Direction::kUp },
    };

    static glm::quat GetOrientationQuat(const glm::vec2 yawPitch)
    {
        const glm::quat yawQuat = glm::angleAxis(yawPitch.x, Direction::kDown);
        const glm::quat pitchQuat = glm::angleAxis(yawPitch.y, Direction::kRight);

        return glm::normalize(yawQuat * pitchQuat);
    }
}

CameraSystem::CameraSystem(Camera* camera_)
    : camera(camera_)
    , parameters(Config::DefaultCamera::kSystemParameters)
    , movementKeyBindings(Config::DefaultCamera::kMovementKeyBindings)
    , speedKeyBindings(Config::DefaultCamera::kSpeedKeyBindings)
{
    const glm::vec3 direction = glm::normalize(camera->GetLocation().target - camera->GetLocation().position);

    const glm::vec2 projection(direction.x, direction.z);

    state.yawPitch.x = glm::atan(direction.x, -direction.z);
    state.yawPitch.y = std::atan2(direction.y, glm::length(projection));

    Engine::AddEventHandler<vk::Extent2D>(EventType::eResize,
            MakeFunction(this, &CameraSystem::HandleResizeEvent));

    Engine::AddEventHandler<KeyInput>(EventType::eKeyInput,
            MakeFunction(this, &CameraSystem::HandleKeyInputEvent));

    Engine::AddEventHandler<glm::vec2>(EventType::eMouseMove,
            MakeFunction(this, &CameraSystem::HandleMouseMoveEvent));

    Engine::AddEventHandler<MouseInput>(EventType::eMouseInput,
            MakeFunction(this, &CameraSystem::HandleMouseInputEvent));
}

void CameraSystem::Process(float deltaSeconds)
{
    if constexpr (Config::kStaticCamera)
    {
        return;
    }

    const glm::vec3 movementDirection = Details::GetOrientationQuat(state.yawPitch) * GetMovementDirection();

    const float speed = parameters.baseSpeed * std::powf(parameters.speedMultiplier, state.speedIndex);
    const float distance = speed * deltaSeconds;

    const glm::vec3 translation = movementDirection * distance;

    camera->SetPosition(camera->GetLocation().position + translation);
    camera->SetTarget(camera->GetLocation().target + translation);

    camera->UpdateViewMatrix();

    if (IsCameraMoved())
    {
        Engine::TriggerEvent(EventType::eCameraUpdate);
    }
}

void CameraSystem::HandleResizeEvent(const vk::Extent2D& extent) const
{
    if (extent.width != 0 && extent.height != 0)
    {
        camera->SetWidth(static_cast<float>(extent.width));
        camera->SetHeight(static_cast<float>(extent.height));
        camera->UpdateProjectionMatrix();
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
            state.speedIndex = static_cast<float>(std::distance(speedKeyBindings.begin(), it));
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

        MovementValue& value = state.movement.at(axis);

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
    }
}

void CameraSystem::HandleMouseMoveEvent(const glm::vec2& position)
{
    if constexpr (Config::kStaticCamera)
    {
        return;
    }

    if (state.rotationEnabled)
    {
        if (lastMousePosition.has_value())
        {
            glm::vec2 delta = position - lastMousePosition.value();
            delta.y = -delta.y;

            state.yawPitch += delta * parameters.sensitivity * Details::kSensitivityReduction;
            state.yawPitch.y = std::clamp(state.yawPitch.y, -Details::kPitchLimitRad, Details::kPitchLimitRad);

            const glm::quat orientation = Details::GetOrientationQuat(state.yawPitch);
            const glm::vec3 direction = orientation * Direction::kForward;

            camera->SetDirection(glm::normalize(direction));
        }

        lastMousePosition = position;

        Engine::TriggerEvent(EventType::eCameraUpdate);
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
            state.rotationEnabled = true;
            break;
        case MouseButtonAction::eRelease:
            state.rotationEnabled = false;
            break;
        default:
            break;
        }
    }
}

bool CameraSystem::IsCameraMoved() const
{
    return std::ranges::any_of(state.movement, [](const auto& entry)
        {
            return entry.second != MovementValue::eNone;
        });
}

glm::vec3 CameraSystem::GetMovementDirection() const
{
    glm::vec3 movementDirection(0.0f);

    for (const auto& [axis, value] : state.movement)
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

#include "Engine/CameraSystem.hpp"

#include "Engine/EngineHelpers.hpp"

CameraSystem::CameraSystem(std::shared_ptr<Camera> camera_, const CameraParameters &parameters_,
        const CameraKeyBindings &keyBindings_)
    : camera(camera_)
    , parameters(parameters_)
    , keyBindings(keyBindings_)
{}

void CameraSystem::Process(float)
{
    camera->Update();
}

void CameraSystem::OnResize(const vk::Extent2D &extent)
{
    camera->AccessProperties().aspect = extent.width / static_cast<float>(extent.height);
}

void CameraSystem::OnKeyInput(Key, KeyAction, ModifierFlags) {}

void CameraSystem::OnMouseInput(MouseButton, MouseButtonAction, ModifierFlags) {}

void CameraSystem::OnMouseMove(const glm::vec2 &position)
{
    if (lastMousePosition.has_value())
    {
        const glm::vec2 delta = position - lastMousePosition.value();

        yawPitch += delta * parameters.sensitivity * 0.001f;

        const glm::quat yawQuat = glm::angleAxis(yawPitch.x, Direction::kUp);
        const glm::quat pitchQuat = glm::angleAxis(yawPitch.y, Direction::kRight);

        const glm::quat orientation = glm::normalize(yawQuat * pitchQuat);
        const glm::vec3 direction = Direction::kFront * orientation;

        camera->AccessProperties().direction = direction;
    }

    lastMousePosition = position;
}

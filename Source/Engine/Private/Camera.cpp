#include "Engine/Camera.hpp"

namespace SCamera
{
    glm::mat4 CalculateViewMatrix(const CameraDescription &description)
    {
        const glm::vec3 target(description.position + description.direction);
        return glm::lookAt(description.position, target, description.up);
    }

    glm::mat4 CalculateProjectionMatrix(const CameraDescription &description)
    {
        const float fov = glm::radians(description.fov) / description.aspect;

        glm::mat4 projectionMatrix = glm::perspective(fov, description.aspect,
                description.zNear, description.zFar);

        projectionMatrix[1][1] = -projectionMatrix[1][1];

        return projectionMatrix;
    }
}

Camera::Camera(const CameraDescription &description_)
    : description(description_)
{
    viewMatrix = SCamera::CalculateViewMatrix(description);
    projectionMatrix = SCamera::CalculateProjectionMatrix(description);
}

void Camera::SetPosition(const glm::vec3 &position)
{
    description.position = position;
    viewMatrix = SCamera::CalculateViewMatrix(description);
}

void Camera::SetDirection(const glm::vec3 &direction)
{
    description.direction = direction;
    viewMatrix = SCamera::CalculateViewMatrix(description);
}

void Camera::SetTarget(const glm::vec3 &target)
{
    description.direction = target - description.position;
    viewMatrix = SCamera::CalculateViewMatrix(description);
}

void Camera::SetUp(const glm::vec3 &up)
{
    description.up = up;
    viewMatrix = SCamera::CalculateViewMatrix(description);
}

void Camera::SetFov(float fov)
{
    description.fov = fov;
    projectionMatrix = SCamera::CalculateProjectionMatrix(description);
}

void Camera::SetAspect(float aspect)
{
    description.aspect = aspect;
    projectionMatrix = SCamera::CalculateProjectionMatrix(description);
}

void Camera::SetZNear(float zNear)
{
    description.zNear = zNear;
    projectionMatrix = SCamera::CalculateProjectionMatrix(description);
}

void Camera::SetZFar(float zFar)
{
    description.zFar = zFar;
    projectionMatrix = SCamera::CalculateProjectionMatrix(description);
}

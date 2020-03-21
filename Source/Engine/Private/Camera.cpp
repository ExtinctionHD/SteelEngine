#include "Engine/Camera.hpp"

namespace SCamera
{
    glm::mat4 CalculateViewMatrix(const CameraProperties &properties)
    {
        return glm::lookAt(properties.position, properties.position + properties.direction, properties.up);
    }

    glm::mat4 CalculateProjectionMatrix(const CameraProperties &properties)
    {
        const float fovRad = glm::radians(properties.fov);

        glm::mat4 projectionMatrix = glm::perspective(fovRad / properties.aspect,
                properties.aspect, properties.zNear, properties.zFar);

        projectionMatrix[1][1] *= -1.0f;

        return projectionMatrix;
    }
}

Camera::Camera(const CameraProperties &properties_)
    : properties(properties_)
{
    viewMatrix = SCamera::CalculateViewMatrix(properties);
    projectionMatrix = SCamera::CalculateProjectionMatrix(properties);
}

void Camera::SetPosition(const glm::vec3 &position)
{
    properties.position = position;
    viewMatrix = SCamera::CalculateViewMatrix(properties);
}

void Camera::SetDirection(const glm::vec3 &direction)
{
    properties.direction = direction;
    viewMatrix = SCamera::CalculateViewMatrix(properties);
}

void Camera::SetTarget(const glm::vec3 &target)
{
    properties.direction = target - properties.position;
    viewMatrix = SCamera::CalculateViewMatrix(properties);
}

void Camera::SetUp(const glm::vec3 &up)
{
    properties.up = up;
    viewMatrix = SCamera::CalculateViewMatrix(properties);
}

void Camera::SetFov(float fov)
{
    properties.fov = fov;
    projectionMatrix = SCamera::CalculateProjectionMatrix(properties);
}

void Camera::SetAspect(float aspect)
{
    properties.aspect = aspect;
    projectionMatrix = SCamera::CalculateProjectionMatrix(properties);
}

void Camera::SetZNear(float zNear)
{
    properties.zNear = zNear;
    projectionMatrix = SCamera::CalculateProjectionMatrix(properties);
}

void Camera::SetZFar(float zFar)
{
    properties.zFar = zFar;
    projectionMatrix = SCamera::CalculateProjectionMatrix(properties);
}

void Camera::UpdateView()
{
    viewMatrix = SCamera::CalculateViewMatrix(properties);
}

void Camera::UpdateProjection()
{
    projectionMatrix = SCamera::CalculateProjectionMatrix(properties);
}

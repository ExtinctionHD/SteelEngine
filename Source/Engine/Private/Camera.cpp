#include "Engine/Camera.hpp"

Camera::Camera(const Description &description_)
    : description(description_)
{
    UpdateViewMatrix();
    UpdateProjectionMatrix();
}

void Camera::SetPosition(const glm::vec3 &position)
{
    description.position = position;
}

void Camera::SetDirection(const glm::vec3 &direction)
{
    description.target = description.position + direction;
}

void Camera::SetTarget(const glm::vec3 &target)
{
    description.target = target;
}

void Camera::SetUp(const glm::vec3 &up)
{
    description.up = up;
}

void Camera::SetFov(float yFov)
{
    description.xFov = yFov;
}

void Camera::SetAspect(float aspect)
{
    description.aspect = aspect;
}

void Camera::SetZNear(float zNear)
{
    description.zNear = zNear;
}

void Camera::SetZFar(float zFar)
{
    description.zFar = zFar;
}

void Camera::UpdateViewMatrix()
{
    viewMatrix = glm::lookAt(description.position, description.target, description.up);
}

void Camera::UpdateProjectionMatrix()
{
    const float yFov = description.xFov / description.aspect;

    projectionMatrix = glm::perspective(yFov, description.aspect,
            description.zNear, description.zFar);

    projectionMatrix[1][1] = -projectionMatrix[1][1];
}

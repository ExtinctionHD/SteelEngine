#include "Engine/Camera.hpp"

#include "Engine/Config.hpp"

Camera::Camera(const Location& location_, const Description& description_)
    : location(location_)
    , description(description_)
{
    UpdateViewMatrix();
    UpdateProjectionMatrix();
}

Camera::Camera(const Description& description_)
    : description(description_)
{
    UpdateViewMatrix();
    UpdateProjectionMatrix();
}

void Camera::SetPosition(const glm::vec3& position)
{
    location.position = position;
}

void Camera::SetDirection(const glm::vec3& direction)
{
    location.target = location.position + direction;
}

void Camera::SetTarget(const glm::vec3& target)
{
    location.target = target;
}

void Camera::SetUp(const glm::vec3& up)
{
    location.up = up;
}

void Camera::SetType(Type type)
{
    description.type = type;
}

void Camera::SetFov(float yFov)
{
    description.yFov = yFov;
}

void Camera::SetWidth(float width)
{
    description.width = width;
}

void Camera::SetHeight(float height)
{
    description.height = height;
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
    viewMatrix = glm::lookAt(location.position, location.target, location.up);
}

void Camera::UpdateProjectionMatrix()
{
    const float zNear = Config::kReverseDepth ? description.zFar : description.zNear;
    const float zFar = Config::kReverseDepth ? description.zNear : description.zFar;

    if (description.type == Type::ePerspective)
    {
        const float aspectRatio = description.width / description.height;

        projectionMatrix = glm::perspective(description.yFov, aspectRatio, zNear, zFar);
    }
    else
    {
        const float halfWidth = description.width * 0.5f;
        const float halfHeight = description.height * 0.5f;

        projectionMatrix = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, zNear, zFar);
    }

    projectionMatrix[1][1] = -projectionMatrix[1][1];
}

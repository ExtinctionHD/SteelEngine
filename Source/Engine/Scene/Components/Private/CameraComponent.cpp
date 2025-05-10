#include "Engine/Scene/Components/CameraComponent.hpp"

#include "Engine/Render/RenderOptions.hpp"
#include "Engine/Scene/Transform.hpp"
#include "Engine/EngineHelpers.hpp"

namespace Details
{
    glm::mat4 ComputePerspectiveMatrix(float yFov, float width, float height, float zNear, float zFar)
    {
        const float aspectRatio = width / height;

        glm::mat4 projMatrix = glm::perspective(yFov, aspectRatio, zNear, zFar);

        projMatrix[1][1] = -projMatrix[1][1];

        return projMatrix;
    }

    glm::mat4 ComputeOrthographicMatrix(float width, float height, float zNear, float zFar)
    {
        const float halfWidth = width * 0.5f;
        const float halfHeight = height * 0.5f;

        glm::mat4 projMatrix = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, zNear, zFar);

        projMatrix[1][1] = -projMatrix[1][1];

        return projMatrix;
    }

    Frustum ComputePerspectiveFrustum(float yFov, float width, float height, float zNear, float zFar)
    {
        using namespace Direction;

        const glm::vec3 nearCenter = kForward * -zNear;
        const glm::vec3 farCenter = kForward * -zFar;

        const float tanHalfVerticalFov = tan(yFov * 0.5f);
        const float tanHalfHorizontalFov = tanHalfVerticalFov * width / height;

        const float halfNearHeight = zNear * tanHalfVerticalFov;
        const float halfNearWidth = zNear * tanHalfHorizontalFov;
        const float halfFarHeight = zFar * tanHalfVerticalFov;
        const float halfFarWidth = zFar * tanHalfHorizontalFov;

        const glm::vec3 nearTopLeft = nearCenter + kUp * halfNearHeight - kRight * halfNearWidth;
        const glm::vec3 nearTopRight = nearCenter + kUp * halfNearHeight + kRight * halfNearWidth;
        const glm::vec3 nearBottomLeft = nearCenter - kUp * halfNearHeight - kRight * halfNearWidth;
        const glm::vec3 nearBottomRight = nearCenter - kUp * halfNearHeight + kRight * halfNearWidth;

        const glm::vec3 farTopLeft = farCenter + kUp * halfFarHeight - kRight * halfFarWidth;
        const glm::vec3 farTopRight = farCenter + kUp * halfFarHeight + kRight * halfFarWidth;
        const glm::vec3 farBottomLeft = farCenter - kUp * halfFarHeight - kRight * halfFarWidth;
        const glm::vec3 farBottomRight = farCenter - kUp * halfFarHeight + kRight * halfFarWidth;

        return Frustum{
            nearTopLeft, nearTopRight, nearBottomRight, nearBottomLeft,
            farTopLeft, farTopRight, farBottomRight, farBottomLeft
        };
    }

    Frustum ComputeOrthographicFrustum(float width, float height, float zNear, float zFar)
    {
        using namespace Direction;

        const glm::vec3 nearCenter = kForward * -zNear;
        const glm::vec3 farCenter = kForward * -zFar;

        const float halfWidth = width * 0.5f;
        const float halfHeight = height * 0.5f;

        const glm::vec3 nearTopLeft = nearCenter + kUp * halfHeight - kRight * halfWidth;
        const glm::vec3 nearTopRight = nearCenter + kUp * halfHeight + kRight * halfWidth;
        const glm::vec3 nearBottomLeft = nearCenter - kUp * halfHeight - kRight * halfWidth;
        const glm::vec3 nearBottomRight = nearCenter - kUp * halfHeight + kRight * halfWidth;

        const glm::vec3 farTopLeft = farCenter + kUp * halfHeight - kRight * halfWidth;
        const glm::vec3 farTopRight = farCenter + kUp * halfHeight + kRight * halfWidth;
        const glm::vec3 farBottomLeft = farCenter - kUp * halfHeight - kRight * halfWidth;
        const glm::vec3 farBottomRight = farCenter - kUp * halfHeight + kRight * halfWidth;

        return Frustum{
            nearTopLeft, nearTopRight, nearBottomRight, nearBottomLeft,
            farTopLeft, farTopRight, farBottomRight, farBottomLeft
        };
    }
}

Frustum operator*(const Transform& t, const Frustum& f)
{
    std::array<glm::vec3, 8> corners;

    std::ranges::transform(f.corners, corners.begin(), [&](const auto& c)
        {
            return glm::vec3(t * glm::vec4(c, 1.0f));
        });

    return Frustum{ corners };
}

glm::mat4 CameraComponent::GetProjMatrix() const
{
    const float effectiveZNear = RenderOptions::reverseDepth ? zFar : zNear;
    const float effectiveZFar = RenderOptions::reverseDepth ? zNear : zFar;

    if (yFov == 0.0f)
    {
        return Details::ComputeOrthographicMatrix(width, height, effectiveZNear, effectiveZFar);
    }

    return Details::ComputePerspectiveMatrix(yFov, width, height, effectiveZNear, effectiveZFar);
}

Frustum CameraComponent::GetLocalFrustum() const
{
    if (yFov == 0.0f)
    {
        return Details::ComputeOrthographicFrustum(width, height, zNear, zFar);
    }

    return Details::ComputePerspectiveFrustum(yFov, width, height, zNear, zFar);
}

Frustum CameraComponent::GetFrustum(const Transform& transform) const
{
    return transform * GetLocalFrustum();
}

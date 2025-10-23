#include "Engine/Scene/Components/CameraComponent.hpp"

#include "Engine/Render/RenderOptions.hpp"
#include "Engine/Scene/Transform.hpp"
#include "Utils/Sphere.hpp"

namespace Details
{
    static glm::mat4 ComputePerspectiveMatrix(float yFov, float width, float height, float zNear, float zFar)
    {
        const float aspectRatio = width / height;

        glm::mat4 projMatrix = glm::perspective(yFov, aspectRatio, zNear, zFar);

        projMatrix[1][1] = -projMatrix[1][1];

        return projMatrix;
    }

    static glm::mat4 ComputeOrthographicMatrix(float width, float height, float zNear, float zFar)
    {
        const float halfWidth = width * 0.5f;
        const float halfHeight = height * 0.5f;

        glm::mat4 projMatrix = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, zNear, zFar);

        projMatrix[1][1] = -projMatrix[1][1];

        return projMatrix;
    }

    static Plane MakePlane(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
    {
        Plane plane;

        plane.normal = glm::normalize(glm::cross(b - a, c - a));
        plane.d = -glm::dot(plane.normal, a);

        return plane;
    }

    static Frustum ComputePerspectiveFrustum(float yFov, float width, float height,
            float zNear, float zFar, const Transform& transform = Transform::kIdentity)
    {
        const glm::vec3 origin = transform.GetTranslation();

        const glm::vec3 forward = transform.GetForward();
        const glm::vec3 right = transform.GetRight();
        const glm::vec3 up = transform.GetUp();

        const glm::vec3 nearCenter = origin + forward * zNear;
        const glm::vec3 farCenter = origin + forward * zFar;

        const float tanHalfVerticalFov = tan(yFov * 0.5f);
        const float tanHalfHorizontalFov = tanHalfVerticalFov * width / height;

        const float halfNearHeight = zNear * tanHalfVerticalFov;
        const float halfNearWidth = zNear * tanHalfHorizontalFov;
        const float halfFarHeight = zFar * tanHalfVerticalFov;
        const float halfFarWidth = zFar * tanHalfHorizontalFov;

        const glm::vec3 nearTopLeft = nearCenter + up * halfNearHeight - right * halfNearWidth;
        const glm::vec3 nearTopRight = nearCenter + up * halfNearHeight + right * halfNearWidth;
        const glm::vec3 nearBottomLeft = nearCenter - up * halfNearHeight - right * halfNearWidth;
        const glm::vec3 nearBottomRight = nearCenter - up * halfNearHeight + right * halfNearWidth;

        const glm::vec3 farTopLeft = farCenter + up * halfFarHeight - right * halfFarWidth;
        const glm::vec3 farTopRight = farCenter + up * halfFarHeight + right * halfFarWidth;
        const glm::vec3 farBottomLeft = farCenter - up * halfFarHeight - right * halfFarWidth;
        const glm::vec3 farBottomRight = farCenter - up * halfFarHeight + right * halfFarWidth;

        const Frustum::Directions directions{
            glm::normalize(farTopLeft - nearTopLeft),
            glm::normalize(farTopRight - nearTopRight),
            glm::normalize(farBottomLeft - nearBottomLeft),
            glm::normalize(farBottomRight - nearBottomRight),
        };

        const Frustum::Corners corners{
            nearTopLeft, nearTopRight, nearBottomLeft, nearBottomRight,
            farTopLeft, farTopRight, farBottomLeft, farBottomRight
        };

        const Frustum::Planes planes{
            MakePlane(corners.nearTopLeft, corners.nearBottomLeft, corners.farBottomLeft),
            MakePlane(corners.nearBottomRight, corners.nearTopRight, corners.farBottomRight),
            MakePlane(corners.nearTopRight, corners.nearTopLeft, corners.farTopLeft),
            MakePlane(corners.nearBottomLeft, corners.nearBottomRight, corners.farBottomRight),
            MakePlane(corners.nearTopLeft, corners.nearTopRight, corners.nearBottomRight),
            MakePlane(corners.farTopRight, corners.farTopLeft, corners.farBottomLeft)
        };

        return Frustum{ directions, corners, planes };
    }

    static Frustum ComputeOrthographicFrustum(float width, float height,
            float zNear, float zFar, const Transform& transform = Transform::kIdentity)
    {
        const glm::vec3 origin = transform.GetTranslation();

        const glm::vec3 forward = transform.GetForward();
        const glm::vec3 right = transform.GetRight();
        const glm::vec3 up = transform.GetUp();

        const glm::vec3 nearCenter = origin + forward * zNear;
        const glm::vec3 farCenter = origin + forward * zFar;

        const float halfWidth = width * 0.5f;
        const float halfHeight = height * 0.5f;

        const glm::vec3 nearTopLeft = nearCenter + up * halfHeight - right * halfWidth;
        const glm::vec3 nearTopRight = nearCenter + up * halfHeight + right * halfWidth;
        const glm::vec3 nearBottomLeft = nearCenter - up * halfHeight - right * halfWidth;
        const glm::vec3 nearBottomRight = nearCenter - up * halfHeight + right * halfWidth;

        const glm::vec3 farTopLeft = farCenter + up * halfHeight - right * halfWidth;
        const glm::vec3 farTopRight = farCenter + up * halfHeight + right * halfWidth;
        const glm::vec3 farBottomLeft = farCenter - up * halfHeight - right * halfWidth;
        const glm::vec3 farBottomRight = farCenter - up * halfHeight + right * halfWidth;

        const Frustum::Directions directions{
            glm::normalize(farTopLeft - nearTopLeft),
            glm::normalize(farTopRight - nearTopRight),
            glm::normalize(farBottomLeft - nearBottomLeft),
            glm::normalize(farBottomRight - nearBottomRight),
        };

        const Frustum::Corners corners{
            nearTopLeft, nearTopRight, nearBottomRight, nearBottomLeft,
            farTopLeft, farTopRight, farBottomRight, farBottomLeft,
        };

        const Frustum::Planes planes{
            MakePlane(corners.nearTopLeft, corners.nearBottomLeft, corners.farBottomLeft),
            MakePlane(corners.nearBottomRight, corners.nearTopRight, corners.farBottomRight),
            MakePlane(corners.nearTopRight, corners.nearTopLeft, corners.farTopLeft),
            MakePlane(corners.nearBottomLeft, corners.nearBottomRight, corners.farBottomRight),
            MakePlane(corners.nearTopLeft, corners.nearTopRight, corners.nearBottomRight),
            MakePlane(corners.farTopRight, corners.farTopLeft, corners.farBottomLeft),
        };

        return Frustum{ directions, corners, planes };
    }
}

bool Frustum::Intersect(const Sphere& sphere) const
{
    if (!sphere.IsValid())
    {
        return false;
    }

    return !std::ranges::any_of(planes.GetArray(), [&](const auto& plane)
        {
            const float distance = glm::dot(plane.normal, sphere.center) + plane.d;

            return distance < -sphere.radius;
        });
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

Frustum CameraComponent::GetFrustum(const Transform& transform) const
{
    if (yFov == 0.0f)
    {
        return Details::ComputeOrthographicFrustum(width, height, zNear, zFar, transform);
    }

    return Details::ComputePerspectiveFrustum(yFov, width, height, zNear, zFar, transform);
}

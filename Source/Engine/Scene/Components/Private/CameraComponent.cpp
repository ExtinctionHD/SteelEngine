#include "Engine/Scene/Components/CameraComponent.hpp"

#include "Engine/Render/RenderOptions.hpp"

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

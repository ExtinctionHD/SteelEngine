#pragma once

#include "Engine/EngineHelpers.hpp"

class Camera
{
public:
    enum Type
    {
        ePerspective,
        eOrthographic
    };

    struct Location
    {
        glm::vec3 position = Vector3::kZero;
        glm::vec3 target = Direction::kForward;
        glm::vec3 up = Direction::kUp;
    };

    struct Description
    {
        Type type;
        float yFov;
        float width;
        float height;
        float zNear;
        float zFar;
    };

    Camera(const Location& location_, const Description& description_);
    Camera(const Description& description_);

    const Location& GetLocation() const { return location; }
    const Description& GetDescription() const { return description; }

    void SetPosition(const glm::vec3& position);
    void SetDirection(const glm::vec3& direction);
    void SetTarget(const glm::vec3& target);
    void SetUp(const glm::vec3& up);

    void SetType(Type type);
    void SetFov(float yFov);
    void SetWidth(float width);
    void SetHeight(float height);
    void SetZNear(float zNear);
    void SetZFar(float zFar);

    const glm::mat4& GetViewMatrix() const { return viewMatrix; }
    const glm::mat4& GetProjectionMatrix() const { return projectionMatrix; }

    void UpdateViewMatrix();
    void UpdateProjectionMatrix();

private:
    Location location;
    Description description;

    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
};

#pragma once
#include "Utils/Helpers.hpp"

class Scene;
class Transform;
struct Sphere;

struct Frustum
{
    struct Directions
    {
        glm::vec3 topLeft;
        glm::vec3 topRight;
        glm::vec3 bottomLeft;
        glm::vec3 bottomRight;

        DEFINE_ARRAY_FUNCTIONS(Directions, glm::vec3)
    };

    struct Corners
    {
        glm::vec3 nearTopLeft;
        glm::vec3 nearTopRight;
        glm::vec3 nearBottomLeft;
        glm::vec3 nearBottomRight;
        glm::vec3 farTopLeft;
        glm::vec3 farTopRight;
        glm::vec3 farBottomLeft;
        glm::vec3 farBottomRight;

        DEFINE_ARRAY_FUNCTIONS(Corners, glm::vec3)
    };

    struct Planes
    {
        Plane left;
        Plane right;
        Plane top;
        Plane bottom;
        Plane near;
        Plane far;

        DEFINE_ARRAY_FUNCTIONS(Planes, Plane)
    };

    Directions directions;
    Corners corners;
    Planes planes;

    bool Intersect(const Sphere& sphere) const;
};

struct CameraComponent
{
    float yFov;
    float width;
    float height;
    float zNear;
    float zFar;

    glm::mat4 GetProjMatrix() const;

    Frustum GetFrustum(const Transform& transform) const;
};

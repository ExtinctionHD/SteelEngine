#pragma once
#include "Utils/Helpers.hpp"

class Scene;
class Transform;
struct Sphere;

struct Frustum
{
    struct Corners
    {
        glm::vec3 nearTopLeft;
        glm::vec3 nearTopRight;
        glm::vec3 nearBottomRight;
        glm::vec3 nearBottomLeft;
        glm::vec3 farTopLeft;
        glm::vec3 farTopRight;
        glm::vec3 farBottomRight;
        glm::vec3 farBottomLeft;

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

    Corners corners;
    Planes planes;

    bool Intersect(const Sphere& sphere) const;
};

Frustum operator*(const Transform& t, const Frustum& f);

struct CameraComponent
{
    float yFov;
    float width;
    float height;
    float zNear;
    float zFar;

    glm::mat4 GetProjMatrix() const;

    Frustum GetLocalFrustum() const;

    Frustum GetFrustum(const Transform& transform) const;
};

#include "Utils/AABBox.hpp"

#include "Engine/Scene/Transform.hpp"
#include "Utils/Sphere.hpp"

void AABBox::Add(const glm::vec3& point)
{
    if (IsValid())
    {
        min = glm::min(point, min);
        max = glm::max(point, max);
    }
    else
    {
        min = point;
        max = point;
    }
}

void AABBox::Add(const Sphere& sphere)
{
    if (IsValid())
    {
        min = glm::min(sphere.center - sphere.radius, min);
        max = glm::max(sphere.center + sphere.radius, max);
    }
    else
    {
        min = sphere.center - sphere.radius;
        max = sphere.center + sphere.radius;
    }
}

void AABBox::Add(const AABBox& other)
{
    if (other.IsValid())
    {
        if (IsValid())
        {
            Add(other.min);
            Add(other.max);
        }
        else
        {
            min = other.min;
            max = other.max;
        }
    }
}

void AABBox::Translate(const glm::vec3& value)
{
    if (IsValid())
    {
        min += value;
        max += value;
    }
}

void AABBox::Scale(const glm::vec3& scale)
{
    if (IsValid())
    {
        const glm::vec3 center = GetCenter();

        min -= center;
        max -= center;

        min *= scale;
        max *= scale;

        min += center;
        max += center;
    }
}

void AABBox::Expand(const glm::vec3& value)
{
    if (IsValid())
    {
        min -= value;
        max += value;
    }
    else
    {
        min = -value;
        max = value;
    }
}

bool AABBox::Intersect(const AABBox& other) const
{
    if (!IsValid() || !other.IsValid())
    {
        return false;
    }

    if ((max.x < other.min.x) || (min.x > other.max.x) ||
        (max.y < other.min.y) || (min.y > other.max.y) ||
        (max.z < other.min.z) || (min.z > other.max.z))
    {
        return false;
    }

    return true;
}

AABBox AABBox::GetTransformed(const Transform& transform) const
{
    AABBox transformedBBox;

    for (const auto& corner : GetCorners())
    {
        transformedBBox.Add(transform * glm::vec4(corner, 1.0f));
    }

    return transformedBBox;
}

std::array<glm::vec3, 8> AABBox::GetCorners() const
{
    std::array<glm::vec3, 8> corners;

    for (size_t i = 0; i < 8; ++i)
    {
        corners[i].x = static_cast<bool>(i & 0b001) ? max.x : min.x;
        corners[i].y = static_cast<bool>(i & 0b010) ? max.y : min.y;
        corners[i].z = static_cast<bool>(i & 0b100) ? max.z : min.z;
    }

    return corners;
}

AABBox operator*(const Transform& t, const AABBox& b)
{
    return b.GetTransformed(t);
}

#include "Utils/Sphere.hpp"

#include "Engine/Scene/Transform.hpp"

void Sphere::Translate(const glm::vec3& offset)
{
    center += offset;
}

void Sphere::Scale(float scale)
{
    radius *= std::abs(scale);
}

void Sphere::Expand(float value)
{
    radius += value;
}

void Sphere::Add(const glm::vec3& point)
{
    const glm::vec3 toPoint = point - center;
    const float dist = glm::length(toPoint);

    if (dist <= radius)
    {
        return;
    }

    const float newRadius = (dist + radius) * 0.5f;

    const glm::vec3 dir = toPoint / dist;

    center += dir * (newRadius - radius);
    radius = newRadius;
}

void Sphere::Add(const Sphere& other)
{
    const glm::vec3 toOther = other.center - center;
    const float dist = glm::length(toOther);

    if (dist + other.radius <= radius)
    {
        return;
    }

    if (dist + radius <= other.radius)
    {
        center = other.center;
        radius = other.radius;
    }
    else
    {
        const float newRadius = (radius + other.radius + dist) * 0.5f;

        if (dist > glm::epsilon<float>())
        {
            const glm::vec3 dir = toOther / dist;

            center = center + dir * (newRadius - radius);
        }

        radius = newRadius;
    }
}

bool Sphere::Intersect(const Sphere& other) const
{
    const float distSq = glm::length2(other.center - center);

    const float radiusSum = radius + other.radius;

    return distSq <= (radiusSum * radiusSum);
}

Sphere Sphere::GetTransformed(const Transform& transform) const
{
    Sphere sphere = *this;
    sphere.Translate(transform.GetTranslation());
    sphere.Scale(glm::compMax(glm::abs(transform.GetScale())));

    return sphere;
}

Sphere operator*(const Transform& t, const Sphere& s)
{
    return s.GetTransformed(t);
}

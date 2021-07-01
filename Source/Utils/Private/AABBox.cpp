#include "Utils/AABBox.hpp"

AABBox::AABBox(const glm::vec3& center, float radius)
{
    Extend(center, radius);
}

AABBox::AABBox(const glm::vec3& p1, const glm::vec3& p2)
    : min(glm::min(p1, p2))
    , max(glm::max(p1, p2))
{}

bool AABBox::IsValid() const
{
    return max.x >= min.x && max.y >= min.y && max.z >= min.z;
}

glm::vec3 AABBox::GetSize() const
{
    if (IsValid())
    {
        return max - min;
    }

    return glm::vec3(0.0f);
}

glm::vec3 AABBox::GetCenter() const
{
    if (IsValid())
    {
        return min + (max - min) * 0.5f;
    }

    return glm::vec3(0.0f);
}

float AABBox::GetLongestEdge() const
{
    return glm::compMax(GetSize());
}

float AABBox::GetShortestEdge() const
{
    return glm::compMin(GetSize());
}

void AABBox::Extend(float value)
{
    if (IsValid())
    {
        min -= glm::vec3(value);
        max += glm::vec3(value);
    }
}

void AABBox::Extend(const glm::vec3& p)
{
    if (IsValid())
    {
        min = glm::min(p, min);
        max = glm::max(p, max);
    }
    else
    {
        min = p;
        max = p;
    }
}

void AABBox::Extend(const glm::vec3& center, float radius)
{
    if (IsValid())
    {
        min = glm::min(center - radius, min);
        max = glm::max(center + radius, max);
    }
    else
    {
        min = center - radius;
        max = center + radius;
    }
}

void AABBox::Extend(const AABBox& bbox)
{
    if (bbox.IsValid())
    {
        Extend(bbox.min);
        Extend(bbox.max);
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

void AABBox::Scale(const glm::vec3& scale, const glm::vec3& origin)
{
    if (IsValid())
    {
        min -= origin;
        max -= origin;

        min *= scale;
        max *= scale;

        min += origin;
        max += origin;
    }
}

AABBox::Intersection AABBox::Intersect(const AABBox& other) const
{
    if (!IsValid() || !other.IsValid())
    {
        return Intersection::eOutside;
    }

    if ((max.x < other.min.x) || (min.x > other.max.x) ||
        (max.y < other.min.y) || (min.y > other.max.y) ||
        (max.z < other.min.z) || (min.z > other.max.z))
    {
        return Intersection::eOutside;
    }

    if ((min.x <= other.min.x) && (max.x >= other.max.x) &&
        (min.y <= other.min.y) && (max.y >= other.max.y) &&
        (min.z <= other.min.z) && (max.z >= other.max.z))
    {
        return Intersection::eInside;
    }

    return Intersection::eIntersect;
}

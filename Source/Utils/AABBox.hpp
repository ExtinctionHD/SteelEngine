#pragma once

class Transform;
struct Sphere;

struct AABBox
{
    glm::vec3 min = glm::vec3(1.0f);
    glm::vec3 max = glm::vec3(0.0f);

    bool IsValid() const { return glm::all(glm::greaterThanEqual(max, min)); }

    glm::vec3 GetSize() const { return max - min; }

    glm::vec3 GetCenter() const { return (min + max) * 0.5f; }

    float GetLongestEdge() const { return glm::compMax(GetSize()); }

    float GetShortestEdge() const { return glm::compMin(GetSize()); }

    std::array<glm::vec3, 8> GetCorners() const;

    void Add(const glm::vec3& point);

    void Add(const Sphere& sphere);

    void Add(const AABBox& other);

    void Translate(const glm::vec3& value);

    void Scale(const glm::vec3& scale);

    void Expand(const glm::vec3& value);

    bool Intersect(const AABBox& other) const;

    AABBox GetTransformed(const Transform& transform) const;
};

AABBox operator*(const Transform& t, const AABBox& b);

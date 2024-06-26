#pragma once

class AABBox
{
public:
    enum class Intersection
    {
        eInside,
        eIntersect,
        eOutside
    };

    AABBox() = default;
    AABBox(const glm::vec3& center, float radius);
    AABBox(const glm::vec3& point1, const glm::vec3& point2);

    bool IsValid() const;

    const glm::vec3& GetMin() const { return min; }
    const glm::vec3& GetMax() const { return max; }

    glm::vec3 GetSize() const;

    glm::vec3 GetCenter() const;

    float GetLongestEdge() const;

    float GetShortestEdge() const;

    std::array<glm::vec3, 8> GetCorners() const;

    void Extend(float value);

    void Extend(const glm::vec3& value);

    void Add(const glm::vec3& point);

    void Add(const glm::vec3& center, float radius);

    void Add(const AABBox& bbox);

    void Translate(const glm::vec3& value);

    void Scale(const glm::vec3& scale, const glm::vec3& origin);

    Intersection Intersect(const AABBox& other) const;

    AABBox GetTransformed(const glm::mat4& transform) const;

private:
    glm::vec3 min = glm::vec3(1.0f);
    glm::vec3 max = glm::vec3(-1.0f);
};

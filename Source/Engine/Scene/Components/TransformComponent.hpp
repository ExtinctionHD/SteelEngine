#pragma once

#include "Utils/Transform.hpp"

class TransformComponent
{
public:
    const Transform& GetLocalTransform() const { return localTransform; }

    const Transform& GetWorldTransform() const { return worldTransform; }

    bool HasBeenModified() const { return modified; }

    bool HasBeenUpdated() const { return updated; }

    void SetLocalTransform(const Transform& transform);

    void SetLocalTranslation(const glm::vec3& translation);

    void SetLocalRotation(const glm::quat& rotation);

    void SetLocalScale(const glm::vec3& scale);

private:
    Transform localTransform;
    Transform worldTransform;

    bool modified = true;
    bool updated = true;

    friend class TransformSystem;
};

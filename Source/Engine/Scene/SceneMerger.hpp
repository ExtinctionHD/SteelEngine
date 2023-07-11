#pragma once

class Scene;

class SceneMerger
{
public:
    SceneMerger(Scene&& srcScene_, Scene& dstScene_, entt::entity dstSpawn_);

private:
    Scene&& srcScene;
    Scene& dstScene;

    entt::entity dstSpawn;

    std::map<entt::entity, entt::entity> entities;

    int32_t textureOffset = 0;
    uint32_t materialOffset = 0;
    uint32_t primitiveOffset = 0;

    void MergeTextureStorageComponents() const;

    void MergeMaterialStorageComponents() const;

    void MergeGeometryStorageComponents() const;

    void AddEntities();

    void AddHierarchyComponent(entt::entity srcEntity, entt::entity dstEntity) const;

    void AddTransformComponent(entt::entity srcEntity, entt::entity dstEntity) const;

    void AddRenderComponent(entt::entity srcEntity, entt::entity dstEntity) const;

    void AddCameraComponent(entt::entity srcEntity, entt::entity dstEntity) const;

    void AddLightComponent(entt::entity srcEntity, entt::entity dstEntity) const;

    void AddEnvironmentComponent(entt::entity srcEntity, entt::entity dstEntity) const;
};

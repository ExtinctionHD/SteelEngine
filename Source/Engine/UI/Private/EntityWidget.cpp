#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>

#include "Engine/UI/EntityWidget.hpp"

#include "Engine/Scene/Scene.hpp"
#include "Engine/Scene/Components/Components.hpp"

namespace Details
{
    static constexpr float kTranslationSpeed = 0.01f;
    static constexpr float kRotationSpeed = 0.1f;
    static constexpr float kScaleSpeed = 0.01f;

    static constexpr float kMinScale = 0.001f;

    static bool IsValidEntity(const Scene& scene, entt::entity entity)
    {
        return entity != entt::null && scene.valid(entity);
    }

    static entt::entity GetDefaultEntity(const Scene& scene)
    {
        if (scene.empty())
        {
            return entt::null;
        }

        return scene.view<HierarchyComponent>().front();
    }

    static std::string BuildNameInput(std::string name)
    {
        ImGui::InputText("Name", &name, ImGuiInputTextFlags_AutoSelectAll);

        return name;
    }

    static void BuildNameView(Scene& scene, entt::entity entity)
    {
        if (auto* nc = scene.try_get<NameComponent>(entity))
        {
            nc->name = BuildNameInput(nc->name);
        }
        else
        {
            const std::string defaultName = SceneHelpers::GetDefaultName(entity);

            const std::string name = BuildNameInput(defaultName);

            if (name != defaultName)
            {
                scene.emplace<NameComponent>(entity, name);
            }
        }
    }

    static glm::vec3 BuildTranslationView(glm::vec3 translation)
    {
        ImGui::DragFloat3("Translation", glm::value_ptr(translation), kTranslationSpeed);

        return translation;
    }

    static glm::quat BuildRotationView(glm::quat rotation)
    {
        glm::vec3 pitchYawRoll = glm::degrees(glm::eulerAngles(rotation));

        if (pitchYawRoll.x >= 180.0f)
        {
            pitchYawRoll.x = -180.0f;
        }
        if (pitchYawRoll.z >= 180.0f)
        {
            pitchYawRoll.z = -180.0f;
        }
        
        ImGui::DragFloat3("Rotation", glm::value_ptr(pitchYawRoll), kRotationSpeed);

        return glm::quat(glm::radians(pitchYawRoll));
    }

    static glm::vec3 BuildScaleView(glm::vec3 scale)
    {
        ImGui::DragFloat3("Scale", glm::value_ptr(scale), kScaleSpeed, kMinScale);

        scale = glm::max(scale, glm::vec3(kMinScale));

        return scale;
    }

    static void BuildWorldTransformView(const Transform& transform)
    {
        ImGui::Text("World Transform");

        ImGui::PushID("World Transform");

        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);

        BuildTranslationView(transform.GetTranslation());
        BuildRotationView(transform.GetRotation());
        BuildScaleView(transform.GetScale());

        ImGui::PopItemFlag();

        ImGui::PopID();
    }

    static void BuildLocalTransformView(Transform& transform)
    {
        ImGui::Text("Local Transform");

        ImGui::PushID("Local Transform");

        const glm::vec3 translation = BuildTranslationView(transform.GetTranslation());
        const glm::quat rotation = BuildRotationView(transform.GetRotation());
        const glm::vec3 scale = BuildScaleView(transform.GetScale());

        transform.SetTranslation(translation);
        transform.SetRotation(rotation);
        transform.SetScale(scale);

        ImGui::PopID();
    }

    static void BuildTransformView(Scene& scene, entt::entity entity)
    {
        auto& tc = scene.get<TransformComponent>(entity);

        Transform localTransform = tc.GetLocalTransform();

        BuildLocalTransformView(localTransform);

        tc.SetLocalTransform(localTransform);

        BuildWorldTransformView(tc.GetWorldTransform());
    }
}

EntityWidget::EntityWidget()
    : ImGuiWidget("Entity")
{}

void EntityWidget::BuildInternal(Scene* scene, float)
{
    if (!scene)
    {
        return;
    }
    
    if (!Details::IsValidEntity(*scene, context.selectedEntity))
    {
        context.selectedEntity = Details::GetDefaultEntity(*scene);
    }

    if (context.selectedEntity != entt::null)
    {
        Details::BuildNameView(*scene, context.selectedEntity);

        ImGui::Separator();

        Details::BuildTransformView(*scene, context.selectedEntity);
    }
}

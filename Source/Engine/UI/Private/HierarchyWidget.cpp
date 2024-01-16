#include <imgui.h>

#include "Engine/UI/HierarchyWidget.hpp"

#include "Engine/Scene/Scene.hpp"
#include "Engine/Scene/Components/Components.hpp"

namespace Details
{
    static void BuildTreeNode(const Scene& scene, entt::entity entity, entt::entity& selectedEntity)
    {
        const auto& hc = scene.get<HierarchyComponent>(entity);

        ImGuiTreeNodeFlags flags = 0;

        flags |= ImGuiTreeNodeFlags_DefaultOpen;
        flags |= ImGuiTreeNodeFlags_OpenOnArrow;

        if (entity == selectedEntity)
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        if (hc.GetChildren().empty())
        {
            flags |= ImGuiTreeNodeFlags_Leaf;
        }

        const std::string displayName = SceneHelpers::GetDisplayName(scene, entity);

        if (ImGui::TreeNodeEx(displayName.c_str(), flags))
        {
            if (ImGui::IsItemClicked())
            {
                selectedEntity = entity;
            }

            for (const auto& child : hc.GetChildren())
            {
                BuildTreeNode(scene, child, selectedEntity);
            }

            ImGui::TreePop();
        }
    }

    static void BuildHierarchyView(const Scene& scene, entt::entity& selectedEntity)
    {
        scene.each([&](entt::entity entity)
            {
                const auto& hc = scene.get<HierarchyComponent>(entity);

                if (hc.GetParent() == entt::null)
                {
                    BuildTreeNode(scene, entity, selectedEntity);
                }
            });
    }
}

HierarchyWidget::HierarchyWidget()
    : ImGuiWidget("Hierarchy")
{}

void HierarchyWidget::BuildInternal(Scene* scene, float)
{
    if (!scene)
    {
        return;
    }

    Details::BuildHierarchyView(*scene, context.selectedEntity);
}

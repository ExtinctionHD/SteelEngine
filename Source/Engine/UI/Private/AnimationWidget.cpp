#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include "Engine/UI/AnimationWidget.hpp"

#include "Engine/Scene/Scene.hpp"
#include "Engine/Scene/Components/AnimationComponent.hpp"

namespace Details
{
    using AnimationComponents = std::map<entt::entity, AnimationComponent&>;

    static AnimationComponents CollectAnimationComponents(Scene& scene)
    {
        AnimationComponents animationComponentMap;

        if (auto* ac = scene.ctx().find<AnimationComponent>())
        {
            animationComponentMap.emplace(entt::null, *ac);
        }

        for (auto&& [entity, ac] : scene.view<AnimationComponent>().each())
        {
            animationComponentMap.emplace(entity, ac);
        }

        return animationComponentMap;
    }

    static bool ContainsSelectedAnimation(const AnimationComponents& components,
            const Animation* selectedAnimation)
    {
        for (const auto& [entity, component] : components)
        {
            for (const auto& animation : component.animations)
            {
                if (&animation == selectedAnimation)
                {
                    return true;
                }
            }
        }

        return false;
    }

    static Animation* GetDefaultAnimation(const AnimationComponents& components)
    {
        if (!components.empty())
        {
            auto& [entity, component] = *components.begin();

            if (!component.animations.empty())
            {
                return &component.animations.front();
            }
        }

        return nullptr;
    }

    static Animation* BuildAnimationComponentView(const std::string& name,
            AnimationComponent& component, Animation* selectedAnimation)
    {
        if (component.animations.empty())
        {
            return selectedAnimation;
        }

        ImGuiTreeNodeFlags flags = 0;

        flags |= ImGuiTreeNodeFlags_DefaultOpen;
        flags |= ImGuiTreeNodeFlags_OpenOnArrow;

        if (ImGui::TreeNodeEx(name.c_str(), flags))
        {
            for (auto& animation : component.animations)
            {
                ImGuiTreeNodeFlags animationFlags = flags;

                animationFlags |= ImGuiTreeNodeFlags_Leaf;

                if (&animation == selectedAnimation)
                {
                    animationFlags |= ImGuiTreeNodeFlags_Selected;
                }

                if (ImGui::TreeNodeEx(animation.name.c_str(), animationFlags))
                {
                    if (ImGui::IsItemClicked())
                    {
                        selectedAnimation = &animation;
                    }

                    ImGui::TreePop();
                }
            }

            ImGui::TreePop();
        }

        return selectedAnimation;
    }

    static Animation* BuildAnimationBrowser(const Scene& scene,
            AnimationComponents& components, Animation* selectedAnimation)
    {
        if (ImGui::BeginTabItem("Browser"))
        {
            ImGui::BeginChild("BrowserChild", ImVec2(-1.0f, -1.0f));

            for (auto& [entity, component] : components)
            {
                if (entity != entt::null)
                {
                    const std::string entityName = SceneHelpers::GetDisplayName(scene, entity);

                    selectedAnimation = BuildAnimationComponentView(entityName, component, selectedAnimation);
                }
                else
                {
                    selectedAnimation = BuildAnimationComponentView("context", component, selectedAnimation);
                }
            }

            ImGui::EndChild();

            ImGui::EndTabItem();
        }

        return selectedAnimation;
    }

    static std::string GetProgressBarOverlay(float time, float duration)
    {
        const uint32_t timeMinutes = static_cast<uint32_t>(time / 60.0f);
        const uint32_t timeSeconds = static_cast<uint32_t>(time - std::floor(time / 60.0f));
        const uint32_t timeMiliseconds = static_cast<uint32_t>((time - std::floor(time)) * 1000.0f);

        const std::string timeString = std::format("{:01}:{:02}:{:03}", timeMinutes, timeSeconds, timeMiliseconds);

        const uint32_t durationMinutes = static_cast<uint32_t>(duration / 60.0f);
        const uint32_t durationSeconds = static_cast<uint32_t>(duration - std::floor(time / 60.0f));
        const uint32_t durationMiliseconds = static_cast<uint32_t>((duration - std::floor(duration)) * 1000.0f);

        const std::string durationString = std::format("{:01}:{:02}:{:03}", durationMinutes, durationSeconds,
                durationMiliseconds);

        return std::format("{} / {}", timeString, durationString);
    }

    static void BuildAnimationPlayer(Animation* animation)
    {
        if (ImGui::BeginTabItem("Player"))
        {
            ImGui::BeginChild("BrowserChild", ImVec2(-1.0f, -1.0f));

            if (animation)
            {
                {
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Name");
                    ImGui::SameLine(80);

                    ImGui::PushItemWidth(-1.0f);
                    ImGui::InputText("##Name", &animation->name, ImGuiInputTextFlags_AutoSelectAll);
                    ImGui::PopItemWidth();
                }

                {
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Looped");
                    ImGui::SameLine(80);

                    bool looped = animation->looped;
                    ImGui::Checkbox("##Looped", &looped);
                    animation->looped = looped;
                }

                {
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Reverse");
                    ImGui::SameLine(80);

                    bool reverse = animation->reverse;
                    ImGui::Checkbox("##Reverse", &reverse);
                    animation->reverse = reverse;
                }

                {
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Speed");
                    ImGui::SameLine(80);

                    ImGui::PushItemWidth(-1.0f);
                    ImGui::SliderFloat("##Speed", &animation->speed, 0.1f, 10.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
                    ImGui::PopItemWidth();
                }

                {
                    const float progress = animation->time / animation->duration;
                    const std::string overlay = GetProgressBarOverlay(animation->time, animation->duration);
                    ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f), overlay.c_str());
                }

                {
                    if (animation->active)
                    {
                        if (ImGui::Button("Pause", ImVec2(50, 0)))
                        {
                            animation->Pause();
                        }
                    }
                    else
                    {
                        if (ImGui::Button("Play", ImVec2(50, 0)))
                        {
                            animation->Play();
                        }
                    }

                    ImGui::SameLine(60);

                    if (ImGui::Button("Reset", ImVec2(50, 0)))
                    {
                        animation->Reset();
                    }
                }

                animation->update = true;
            }

            ImGui::EndChild();

            ImGui::EndTabItem();
        }
    }
}

AnimationWidget::AnimationWidget()
    : ImGuiWidget("Animation")
{}

void AnimationWidget::BuildInternal(Scene* scene, float)
{
    if (!scene)
    {
        return;
    }

    Details::AnimationComponents components = Details::CollectAnimationComponents(*scene);

    if (!Details::ContainsSelectedAnimation(components, context.selectedAnimation))
    {
        context.selectedAnimation = nullptr;
    }

    if (!context.selectedAnimation)
    {
        context.selectedAnimation = Details::GetDefaultAnimation(components);
    }

    ImGui::BeginTabBar("AnimationTabBar", ImGuiTabBarFlags_Reorderable);

    context.selectedAnimation = Details::BuildAnimationBrowser(*scene, components, context.selectedAnimation);

    Details::BuildAnimationPlayer(context.selectedAnimation);

    ImGui::EndTabBar();
}

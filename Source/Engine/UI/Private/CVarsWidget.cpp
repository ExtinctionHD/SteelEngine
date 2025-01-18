#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include "Engine/UI/CVarsWidget.hpp"

#include "Engine/ConsoleVariable.hpp"
#include "Engine/Engine.hpp"

namespace Details
{
    template <class T>
    static void BuildCVarsView(const ImGuiTextFilter& filter)
    {
        static_assert(
            std::is_same_v<T, bool> ||
            std::is_same_v<T, float> ||
            std::is_same_v<T, int32_t> ||
            std::is_same_v<T, std::string>);

        ConsoleVariable<T>::Enumerate([&](ConsoleVariable<T>& cvar)
            {
                const std::string& key = cvar.GetKey();

                if (filter.PassFilter(key.c_str()))
                {
                    T value = cvar.GetValue();

                    if constexpr (std::is_same_v<T, bool>)
                    {
                        ImGui::Checkbox(key.c_str(), &value);
                    }
                    if constexpr (std::is_same_v<T, int32_t>)
                    {
                        ImGui::PushItemWidth(70.0f);
                        ImGui::DragInt(key.c_str(), &value);
                        ImGui::PopItemWidth();
                    }
                    if constexpr (std::is_same_v<T, float>)
                    {
                        ImGui::PushItemWidth(120.0f);
                        ImGui::DragFloat(key.c_str(), &value);
                        ImGui::PopItemWidth();
                    }
                    if constexpr (std::is_same_v<T, std::string>)
                    {
                        ImGui::PushItemWidth(300.0f);
                        ImGui::InputText(key.c_str(), &value);
                        ImGui::PopItemWidth();
                    }
                    
                    cvar.SetValue(value);
                }
            });
    }

    static void BuildCVarsView(const ImGuiTextFilter& filter)
    {
        const float height = ImGui::GetWindowHeight() * 0.25f;

        ImGui::BeginChild("CVars", ImVec2(0, height), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

        Details::BuildCVarsView<bool>(filter);

        Details::BuildCVarsView<float>(filter);

        Details::BuildCVarsView<int32_t>(filter);

        Details::BuildCVarsView<std::string>(filter);

        ImGui::EndChild();
    }
}

CVarsWidget::CVarsWidget()
    : ImGuiWidget("CVars")
{}

void CVarsWidget::BuildInternal(Scene*, float)
{
    static ImGuiTextFilter filter;

    filter.Draw("Filter", 250.0f);

    Details::BuildCVarsView(filter);

    static std::string configPath = Engine::kConfigPath;

    ImGui::PushItemWidth(250.0f);
    ImGui::InputText("##ConfigPath", &configPath);
    ImGui::PopItemWidth();

    ImGui::SameLine(0.0f, 4.0f);

    if (ImGui::Button("Save"))
    {
        CVarHelpers::SaveConfig(Filepath(configPath));
    }
}

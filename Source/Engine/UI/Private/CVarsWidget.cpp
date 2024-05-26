#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include "Engine/UI/CVarsWidget.hpp"

#include "Engine/ConsoleVariable.hpp"

namespace Details
{
    static constexpr float kDragIntWidth = 70.0f;
    static constexpr float kDragFloatWidth = 120.0f;
    static constexpr float kInputTextWidth = 300.0f;
    static constexpr float kReadOnlyAlpha = 0.5f;

    template <class T>
    static void BuildCVarsView()
    {
        static_assert(
            std::is_same_v<T, int> ||
            std::is_same_v<T, bool> ||
            std::is_same_v<T, float> ||
            std::is_same_v<T, std::string>);
        
        ConsoleVariable<T>::Enumerate([&](ConsoleVariable<T>& cvar)
            {
                const bool readOnly = cvar.HasFlag(CVarFlagBits::eReadOnly);

                if (readOnly)
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, kReadOnlyAlpha);
                }
                
                T value = cvar.GetValue();

                if constexpr (std::is_same_v<T, bool>)
                {
                    ImGui::Checkbox(cvar.GetKey().c_str(), &value);
                }
                if constexpr (std::is_same_v<T, int>)
                {
                    ImGui::PushItemWidth(kDragIntWidth);
                    ImGui::DragInt(cvar.GetKey().c_str(), &value);
                    ImGui::PopItemWidth();
                }
                if constexpr (std::is_same_v<T, float>)
                {
                    ImGui::PushItemWidth(kDragFloatWidth);
                    ImGui::DragFloat(cvar.GetKey().c_str(), &value);
                    ImGui::PopItemWidth();
                }
                if constexpr (std::is_same_v<T, std::string>)
                {
                    ImGui::PushItemWidth(kInputTextWidth);
                    ImGui::InputText(cvar.GetKey().c_str(), &value);
                    ImGui::PopItemWidth();
                }

                if (readOnly)
                {
                    ImGui::PopStyleVar();
                }
                else
                {
                    cvar.SetValue(value);
                }
            });
    }
}

CVarsWidget::CVarsWidget()
    : ImGuiWidget("CVars")
{}

void CVarsWidget::BuildInternal(Scene*, float)
{
    Details::BuildCVarsView<bool>();

    Details::BuildCVarsView<int>();

    Details::BuildCVarsView<float>();

    Details::BuildCVarsView<std::string>();
}

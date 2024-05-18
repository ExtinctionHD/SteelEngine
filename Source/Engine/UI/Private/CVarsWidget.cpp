#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include "Engine/UI/CVarsWidget.hpp"

#include "Engine/ConsoleVariable.hpp"

namespace Details
{
    template <class T>
    void BuildCVarsView()
    {
        static_assert(
            std::is_same_v<T, int> ||
            std::is_same_v<T, bool> ||
            std::is_same_v<T, float> ||
            std::is_same_v<T, std::string>);

        ConsoleVariable<T>::Enumerate([&](ConsoleVariable<T>& cvar)
            {
                const std::string key = cvar.GetKey();

                T value = cvar.GetValue();

                if constexpr (std::is_same_v<T, bool>)
                {
                    ImGui::Checkbox(key.c_str(), &value);
                }
                else if constexpr (std::is_same_v<T, int>)
                {
                    ImGui::DragInt(key.c_str(), &value);
                }
                else if constexpr (std::is_same_v<T, float>)
                {
                    ImGui::DragFloat(key.c_str(), &value);
                }
                else if constexpr (std::is_same_v<T, std::string>)
                {
                    ImGui::InputText(key.c_str(), &value);
                }

                if (!cvar.HasFlag(CVarFlagBits::eReadOnly))
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

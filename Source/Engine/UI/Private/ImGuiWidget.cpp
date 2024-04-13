#include <imgui.h>

#include "Engine/UI/ImGuiWidget.hpp"

ImGuiWidget::Context ImGuiWidget::context;

ImGuiWidget::ImGuiWidget(const std::string& name_)
    : name(name_)
{}

void ImGuiWidget::Build(Scene* scene, float deltaSeconds)
{
    if (ImGui::CollapsingHeader(name.c_str()))
    {
        BuildInternal(scene, deltaSeconds);
    }
}

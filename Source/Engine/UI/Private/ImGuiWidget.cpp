#include <imgui.h>

#include "Engine/UI/ImGuiWidget.hpp"

ImGuiWidget::Context ImGuiWidget::context;

ImGuiWidget::ImGuiWidget(const std::string& name_)
    : name(name_)
{}

void ImGuiWidget::Build(const Scene* scene, float deltaSeconds)
{
    ImGui::Begin(name.c_str());

    BuildInternal(scene, deltaSeconds);

    ImGui::End();
}

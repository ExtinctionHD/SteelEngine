#include <imgui.h>

#include "Engine/UI/StatWidget.hpp"

#include "Utils/Helpers.hpp"

StatWidget::StatWidget()
    : ImGuiWidget("Stats")
{}

void StatWidget::BuildInternal(Scene*, float deltaSeconds)
{
    const float frameTime = deltaSeconds / Metric::kMili;

    const float fps = 1.0f / deltaSeconds; // TODO implement fps averaging

    ImGui::Text("%s", std::format("Frame time: {:.2f} ms ({:.1f} FPS)", frameTime, fps).c_str());
}

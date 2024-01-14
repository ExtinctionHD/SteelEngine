#include <imgui.h>

#include "Engine/UI/StatViewer.hpp"

#include "Utils/Helpers.hpp"

StatViewer::StatViewer()
    : ImGuiWidget("Stat Viewer")
{}

void StatViewer::BuildInternal(const Scene*, float deltaSeconds)
{
    const float frameTime = deltaSeconds / Metric::kMili;

    const float fps = 1.0f / deltaSeconds;

    ImGui::Text("%s", std::format("Frame time: {:.2f} ms ({:.1f} FPS)", frameTime, fps).c_str());
}

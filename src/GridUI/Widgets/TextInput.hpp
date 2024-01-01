#pragma once

#include <span>
#include <vector>
#include <string>

#include "GridUI/Colors.hpp"
#include "imgui.h"
#include "util.h"

#include "GridUI/GridUI.hpp"

inline bool DrawInputFloat(GridUI const& ui, char const* label, float* value)
{
    ImGui::PushID("InputFloat");
    ImGui::PushID(label);
    ColorScheme const& colors = ui.GetColorScheme();

    ImRect const& dst = ui.GetComponentRect();

    ImGui::PushStyleColor(ImGuiCol_FrameBg, colors.display.background);
    ImGui::PushStyleColor(ImGuiCol_Text, colors.display.text);
    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, colors.selection.background);

    float offset = ImGui::GetStyle().FramePadding.y + ImGui::GetTextLineHeight() * 0.5f;
    ImGui::SetCursorScreenPos(ImVec2(dst.Min.x, dst.GetCenter().y - offset));
    ImGui::PushItemWidth(dst.GetWidth());
    ImGui::InputFloat("##value", value, 0.0f, 0.0f, "%.2f",
                        ImGuiInputTextFlags_None);
    ImGui::PopItemWidth();

    ImGui::PopStyleColor(3);

    ImGui::PopID();
    ImGui::PopID();

    return true;
}

#pragma once

#include <span>
#include <vector>
#include <string>

#include "GridUI/Colors.hpp"
#include "imgui.h"
#include "util.h"

#include "GridUI/GridUI.hpp"
/**
    * Toggle is a widget for selecting 1 of N options
    * [  | x |  |   ]
    */
enum class ToggleOrientation {
    kHorizontal, kVertical
};

struct ToggleParams {
    uint32_t numOptions;
    std::vector<std::string> tooltips;
    float height;
    float sideMargin;
};

inline bool DrawToggle(GridUI const& ui, char const* label, uint32_t* value, ToggleParams const& params)
{
    ImRect const& dst = ui.GetComponentRect();
    auto drawList = ImGui::GetWindowDrawList();

    float dstHeight = dst.GetHeight();
    float h = std::min(dstHeight, params.height);
    float offset = std::max(dstHeight - h, 0.0f) / 2.0f;

    ImVec2 topLeft = {dst.Min.x + params.sideMargin, dst.Min.y + offset};
    ImVec2 btmRight = {dst.Max.x - params.sideMargin, dst.Min.y + offset + h};
    ImRect outlineRect = {topLeft, btmRight};

    float optWidth = outlineRect.GetWidth() / params.numOptions;
    ImRect optRect = {topLeft, topLeft + ImVec2{optWidth, h}};

    ImGui::PushID("Toggle");

    ColorScheme const& colors = ui.GetColorScheme();

    ImU32 backgroundColor =   colors.inactive.primary;
    ImU32 outlineColor =   colors.inactive.secondary;
    ImU32 selectionColor = colors.selection.primary;

    for (uint32_t optIdx = 0; optIdx < params.numOptions; ++optIdx) {
        ImGui::PushID(optIdx);
        ImGui::SetCursorScreenPos(optRect.Min);
        bool pressed = ImGui::InvisibleButton(label, optRect.GetSize(), ImGuiButtonFlags_MouseButtonLeft);
        bool hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_None);
        ImGui::PopID();

        if (hovered) {
            if (ImGui::BeginTooltip())
            {
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                ImGui::TextUnformatted(params.tooltips.at(optIdx).c_str());
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }
        }

        if (pressed) {
            *value = optIdx;
        }

        ImVec2 offset = {1, 1};
        if (*value == optIdx) {
            drawList->AddRectFilled(optRect.Min + offset, optRect.Max - offset, selectionColor);
            drawList->AddRect(optRect.Min, optRect.Max, outlineColor);
        } else {
            drawList->AddRectFilled(optRect.Min, optRect.Max, backgroundColor);
            drawList->AddRect(optRect.Min, optRect.Max, outlineColor);
        }

        optRect.TranslateX(optWidth);
    }

    drawList->AddRect(outlineRect.Min, outlineRect.Max, outlineColor);
    ImGui::PopID();

    return 0;
}

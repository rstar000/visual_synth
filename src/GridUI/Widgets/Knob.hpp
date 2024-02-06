#pragma once

#include <iostream>

#include "GridUI/Colors.hpp"
#include "imgui.h"
#include "imgui_knobs.h"

#include "GridUI/GridUI.hpp"

template <typename T>
struct KnobParams {
    T minValue;
    T maxValue;
    T defaultValue;
    const char* format;
    bool useDefault = true;
    uint32_t colorIndex = 0;
};

inline bool DrawKnob(GridUI const& ui, const char* label, float* v, KnobParams<float> const& params) {
    ImRect const& dst = ui.GetComponentRect();
    ImVec2 size = dst.GetSize();
    ImVec2 delta = {0, 0};
    if (size.x < size.y) {
        delta.y = (size.y - size.x) / 2.0f;
    } else {
        delta.x = (size.x - size.y) / 2.0f;
    }
    ImGui::SetCursorScreenPos(dst.Min + delta);
    float minSize = std::min(size.x, size.y);

    int flags = 0;
    // flags |= ImGuiKnobFlags_NoTitle;
    if (params.useDefault) {
        flags |= ImGuiKnobFlags_UseDefaultValue;
    }

    const ColorScheme& colors = ui.GetColorScheme();

    ImGui::PushStyleColor(ImGuiCol_FrameBg, colors.nodeColors.border.normal);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors.range.primary);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors.range.secondary);

    bool valueChanged = ImGuiKnobs::Knob(label, ImVec2(minSize, minSize), v, params.minValue, params.maxValue,
                            params.defaultValue, 0.0f, params.format, ImGuiKnobVariant_Wiper,
                            flags);

    ImGui::PopStyleColor(3);
    return valueChanged;
}

inline bool DrawKnobInt(GridUI const& ui, const char* label, int* v, KnobParams<int> const& params) {
    ImRect const& dst = ui.GetComponentRect();
    ImVec2 size = dst.GetSize();
    ImVec2 delta = {0, 0};
    if (size.x < size.y) {
        delta.y = (size.y - size.x) / 2.0f;
    } else {
        delta.x = (size.x - size.y) / 2.0f;
    }
    ImGui::SetCursorScreenPos(dst.Min + delta);
    float minSize = std::min(size.x, size.y);

    int flags = 0;
    // flags |= ImGuiKnobFlags_NoTitle;
    if (params.useDefault) {
        flags |= ImGuiKnobFlags_UseDefaultValue;
    }

    ColorScheme const& colors = ui.GetColorScheme();
    ImGui::PushStyleColor(ImGuiCol_FrameBg, colors.nodeColors.border.normal);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, colors.range.primary);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors.range.secondary);
    bool valueChanged = ImGuiKnobs::KnobInt(label, ImVec2(minSize, minSize), v, params.minValue, params.maxValue,
                               params.defaultValue, 0.0f, params.format, ImGuiKnobVariant_Wiper,
                               flags);

    ImGui::PopStyleColor(3);
    return valueChanged;
}

template <typename ...Args>
inline bool DrawKnobEx(GridUI& ui, GridComponent const& component, Args ...args) {
    ui.BeginComponent(component);
    bool res = DrawKnob(ui, args...);
    ui.EndComponent();
    return res;
}

template <typename ...Args>
inline bool DrawKnobIntEx(GridUI& ui, GridComponent const& component, Args ...args) {
    ui.BeginComponent(component);
    bool res = DrawKnobInt(ui, args...);
    ui.EndComponent();
    return res;
}

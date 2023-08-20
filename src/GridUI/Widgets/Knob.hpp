#pragma once

#include "imgui.h"
#include "imgui_knobs.h"
#include <iostream>

inline bool DrawKnob(const char* label, ImRect dst, float& v, float vMin, float vMax, float vDefault, const char* format, bool useDefault)
{
    ImVec2 size = dst.GetSize();
    ImVec2 delta = {0,0};
    if (size.x < size.y) {
        delta.y = (size.y - size.x) / 2.0f;
    } else {
        delta.x = (size.x - size.y) / 2.0f;
    }
    ImGui::SetCursorScreenPos(dst.Min + delta);
    float minSize = std::min(size.x, size.y);

    int flags = 0;
    // flags |= ImGuiKnobFlags_NoTitle;
    if (useDefault) {
        flags |= ImGuiKnobFlags_UseDefaultValue;
    }
    return ImGuiKnobs::Knob(label, ImVec2(minSize, minSize), &v, vMin, vMax, vDefault, 0.0f, format, ImGuiKnobVariant_Wiper, flags);
}


inline bool DrawKnobInt(const char* label, ImRect dst, int& v, int vMin, int vMax, int vDefault, const char* format, bool useDefault)
{
    ImVec2 size = dst.GetSize();
    ImVec2 delta = {0,0};
    if (size.x < size.y) {
        delta.y = (size.y - size.x) / 2.0f;
    } else {
        delta.x = (size.x - size.y) / 2.0f;
    }
    ImGui::SetCursorScreenPos(dst.Min + delta);
    float minSize = std::min(size.x, size.y);

    int flags = 0;
    // flags |= ImGuiKnobFlags_NoTitle;
    if (useDefault) {
        flags |= ImGuiKnobFlags_UseDefaultValue;
    }
    return ImGuiKnobs::KnobInt(label, ImVec2(minSize, minSize), &v, vMin, vMax, vDefault, 0.0f, format, ImGuiKnobVariant_Wiper, flags);
}
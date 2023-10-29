#pragma once

#include "imgui.h"
#include <iostream>
#include "GridUI/Colors.hpp"

constexpr float PIN_SIZE = 20.0f;

enum class PinState {
    kNormal,
    kConnected,
    kHovered,
    kSelected
};

enum class PinKind
{
    kIn, kOut
};


inline bool DrawOutPin(const char* label, ImRect dst, ImU32 fillColor, ImU32 borderColor, float thickness = 2.0f)
{
    auto draw_list = ImGui::GetWindowDrawList();
    ImVec2 center = dst.GetCenter();
    draw_list->AddTriangleFilled(
        center + ImVec2(-PIN_SIZE * 0.5f, PIN_SIZE * 0.5f),
        center + ImVec2(-PIN_SIZE * 0.5f, -PIN_SIZE * 0.5f),
        center + ImVec2(PIN_SIZE * 0.5f, 0.0f),
        fillColor);
    draw_list->AddTriangle(
        center + ImVec2(-PIN_SIZE * 0.5f, PIN_SIZE * 0.5f),
        center + ImVec2(-PIN_SIZE * 0.5f, -PIN_SIZE * 0.5f),
        center + ImVec2(PIN_SIZE * 0.5f, 0.0f),
        borderColor, thickness);
    return true;
}

inline bool DrawInPin(const char* label, ImRect dst, ImU32 fillColor, ImU32 borderColor, float thickness = 2.0f)
{
    auto draw_list = ImGui::GetWindowDrawList();
    draw_list->AddCircleFilled(dst.GetCenter(), PIN_SIZE * 0.5f, fillColor);
    draw_list->AddCircle(dst.GetCenter(), PIN_SIZE * 0.5f, borderColor, 0, thickness);
    return true;
}

inline bool DrawPin(const char* label, ImRect dst, PinKind kind, PinState state, ColorScheme::PinColors colors)
{
    auto drawFunc = ((kind == PinKind::kIn) ? DrawInPin : DrawOutPin);
    ColorRGBA fill = colors.fill.normal;
    ColorRGBA border = colors.border.normal;

    if (state == PinState::kConnected) {
        fill = border;
    }

    if (state == PinState::kHovered) {
        fill = colors.fill.hovered;
        border = colors.border.hovered;
    }

    if (state == PinState::kSelected) {
        fill = colors.fill.selected;
        border = colors.border.selected;
    }
    drawFunc(label, dst, fill, border, 2.0f);
    return true;
}

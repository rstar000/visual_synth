#pragma once
#include "GridUI/Colors.hpp"
#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>

#include "GridUI/GridUI.hpp"

struct FaderRectParams {
    float minValue;
    float maxValue;
    const char* format;
    float speed;
    bool highlighted;
};

namespace ImGui
{

inline bool VFader(GridUI const& ui, const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags)
{
    ImGuiWindow* window = GetCurrentWindow();
    ImRect const& dst = ui.GetComponentRect();
    ImVec2 size = dst.GetSize();
    ImGui::SetCursorScreenPos(dst.Min);
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    ImVec2 curPos = window->DC.CursorPos;
    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    float textHeight = label_size.y;

    const ImRect frame_bb_base(curPos, curPos + size);
    const ImRect frame_bb(curPos + ImVec2(0.0f, textHeight), curPos + size - ImVec2(0.0f, textHeight));
    const ImRect bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

    ItemSize(bb, style.FramePadding.y);
    if (!ItemAdd(frame_bb, id))
        return false;

    // Default format string when passing NULL
    if (format == NULL)
        format = DataTypeGetInfo(data_type)->PrintFmt;

    const bool hovered = ItemHoverable(frame_bb, id, g.LastItemData.InFlags);
    const bool clicked = hovered && IsMouseClicked(0, id);
    if (clicked || g.NavActivateId == id)
    {
        if (clicked)
            SetKeyOwner(ImGuiKey_MouseLeft, id);
        SetActiveID(id, window);
        SetFocusID(id, window);
        FocusWindow(window);
        g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Up) | (1 << ImGuiDir_Down);
    }

    // Draw frame
    float compWidth = size.x;
    float baseWidth = 20.0f;
    float grabWidth = 50.0f;
    float d = (compWidth - baseWidth) / 2.0f;

    ImRect base_bb = frame_bb;
    base_bb.Min.x += d;
    base_bb.Max.x -= d;
    const ImU32 frame_col = GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
    RenderNavHighlight(base_bb, id);
    RenderFrame(base_bb.Min, base_bb.Max, frame_col, true, g.Style.FrameRounding);

    // Slider behavior
    ImRect grab_bb;
    const bool value_changed = SliderBehavior(frame_bb, id, data_type, p_data, p_min, p_max, format, flags | ImGuiSliderFlags_Vertical, &grab_bb);
    if (value_changed)
        MarkItemEdited(id);

    ImRect grab_bb_scaled = grab_bb;
    float grabDelta = (size.x - grabWidth) / 2.0f;
    grab_bb_scaled.Min.x += grabDelta;
    grab_bb_scaled.Max.x -= grabDelta;
    // Render grab
    if (grab_bb.Max.y > grab_bb.Min.y)
        window->DrawList->AddRectFilled(grab_bb_scaled.Min, grab_bb_scaled.Max, GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab), style.GrabRounding);

    // Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
    // For the vertical slider we allow centered text to overlap the frame padding
    char value_buf[64];
    const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data, format);
    RenderTextClipped(ImVec2(frame_bb_base.Min.x, frame_bb_base.Min.y + style.FramePadding.y), frame_bb_base.Max, value_buf, value_buf_end, NULL, ImVec2(0.5f, 0.0f));
    if (label_size.x > 0.0f)
        RenderText(ImVec2(frame_bb_base.Min.x + style.ItemInnerSpacing.x, frame_bb_base.Max.y - textHeight - style.FramePadding.y), label);

    return value_changed;
}

inline bool VFaderRectEx(ImVec2 const size, ColorScheme const& colors, const char* label, float* p_data, FaderRectParams params)
{
    ImGuiWindow* window = GetCurrentWindow();

    ImU32 frameBgHovered = colors.hovered.secondary;
    ImU32 frameBgActive = colors.hovered.primary;
    ImU32 sliderGrabActive = colors.hovered.primary;
    ImU32 frameBg = colors.range.secondary;
    ImU32 sliderGrab = colors.range.primary;

    if (params.highlighted) {
        frameBg = colors.selection.secondary;
        sliderGrab = colors.selection.primary;
    } 

    if (window->SkipItems)
        return false;

    ImVec2 const origin = ImGui::GetCursorScreenPos();
    ImRect const dst = {origin, origin + size};

    ImGui::InvisibleButton(label, dst.GetSize());
    auto gid = ImGui::GetID(label);
    ImGuiSliderFlags drag_flags = 0;
    drag_flags |= ImGuiSliderFlags_Vertical;

    bool is_active = ImGui::IsItemActive();
    bool is_hovered = ImGui::IsItemHovered();

    auto speed = params.speed == 0 ? (params.maxValue - params.minValue) / 250.f : params.speed;
    bool value_changed = ImGui::DragBehavior(gid, ImGuiDataType_Float, p_data, speed, &params.minValue, &params.maxValue, params.format, drag_flags);

    ImGuiContext& g = *GImGui;
    const ImGuiID id = window->GetID(label);

    ImRect const& frame_bb = dst;
    float value_frac = std::clamp((*p_data - params.minValue) / (params.maxValue - params.minValue), 0.0f, 1.0f);
    float value_pos = (1.0f - value_frac) * dst.GetHeight();

    const ImRect grab_bb = ImRect(dst.Min + ImVec2(0.0f, value_pos), dst.Max);
    const ImU32 frame_col = GetColorU32(g.ActiveId == id ? frameBgActive : is_hovered ? frameBgHovered : frameBg);
    RenderNavHighlight(frame_bb, id);
    RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, true, g.Style.FrameRounding);

    if (grab_bb.Max.y > grab_bb.Min.y)
        window->DrawList->AddRectFilled(grab_bb.Min, grab_bb.Max, GetColorU32(g.ActiveId == id ? sliderGrabActive : sliderGrab), 0.0f);

    if (is_active || is_hovered)
        ImGui::SetTooltip("%.3f", *p_data);
    return value_changed;
}

inline bool VFaderRect(GridUI const& ui, const char* label, float* p_data, FaderRectParams params)
{
    ImGuiWindow* window = GetCurrentWindow();
    ImRect const& dst = ui.GetComponentRect();
    ColorScheme const& colors = ui.GetColorScheme();

    if (window->SkipItems)
        return false;

    ImGui::SetCursorScreenPos(dst.Min);
    return VFaderRectEx(dst.GetSize(), colors, label, p_data, params);
}

}

/* inline bool DrawFader(GridUI const& ui, const char* label, float& v, float vMin, float vMax, const char* format) */
/* { */
/*     return ImGui::VFader(label, dst.GetSize(), ImGuiDataType_Float, &v, &vMin, &vMax, format, ImGuiSliderFlags_None); */
/* } */


inline bool DrawFaderRect(GridUI const& ui, const char* label, float* v, FaderRectParams const& params)
{
    return ImGui::VFaderRect(ui, label, v, params);
}

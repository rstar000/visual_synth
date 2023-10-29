#pragma once
#include "imgui.h"
#include "imgui_internal.h"

#include "GridUI/Colors.hpp"
#include "GridUI/GridUI.hpp"
#include <algorithm>


namespace ImGui
{
bool DrawCheckboxImpl(ColorScheme const& colors, const char* label, const ImRect& dst, bool& value, bool labelBelow, float labelOffset);
}

inline bool DrawCheckbox(
    const char* label, const ImRect& dst, bool& value, 
    bool labelBelow = true, float labelOffset = 3.0f)
{
    // return ImGui::DrawCheckboxImpl(label, dst, value, labelBelow, labelOffset);
    return false;
}

inline bool DrawCheckbox(
    GridUI const& ui, const char* label, bool& value, 
    bool labelBelow = true, float labelOffset = 3.0f)
{
    return ImGui::DrawCheckboxImpl(ui.GetColorScheme(), label, ui.GetComponentRect(), value, labelBelow, labelOffset);
}


#pragma once
#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>


namespace ImGui
{
bool DrawCheckboxImpl(const char* label, const ImRect& dst, bool& value, bool labelBelow, float labelOffset);
}

inline bool DrawCheckbox(const char* label, const ImRect& dst, bool& value, bool labelBelow = true, float labelOffset = 3.0f)
{
    return ImGui::DrawCheckboxImpl(label, dst, value, labelBelow, labelOffset);
}


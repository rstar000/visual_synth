#include "WidgetUtils.hpp"

#include "imgui.h"
#include "imgui_internal.h"

namespace ImGui {


void RenderTextCenteredImpl(const char* label, ImRect dst)
{
    const ImVec2 labelSize = CalcTextSize(label, NULL, true);
    float dstWidth = dst.GetWidth();

    if (labelSize.x > dstWidth) {
        RenderTextClipped(dst.Min, dst.Max, label, NULL, &labelSize);
    } else {
        ImVec2 origin = {dst.Min.x + (dstWidth - labelSize.x) * 0.5f, dst.Min.y};
        RenderText(origin, label, NULL, true);
    }
}

}

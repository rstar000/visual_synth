#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "Checkbox.hpp"

#include "util.h"
#include "GridUI/Widgets/WidgetUtils.hpp"

namespace ImGui {

constexpr float CHECKBOX_SIZE = 20.0f;
constexpr float CHECKBOX_PADDING = 2.0f;

bool DrawCheckboxImpl(const char* label, const ImRect& dst, bool& value, bool labelBelow, float labelOffset) 
{
    const ImVec2 outlineOrigin = {dst.Min.x + (dst.GetWidth() - CHECKBOX_SIZE) * 0.5f,
                                  dst.Min.y + (dst.GetHeight() - CHECKBOX_SIZE) * 0.5f};
    auto drawList = ImGui::GetWindowDrawList();

    ImU32 outlineColor = IM_COL32(255, 0, 0, 255);
    ImVec2 checkboxSize = {CHECKBOX_SIZE, CHECKBOX_SIZE};
    ImRect checkboxRect = ImRect{outlineOrigin, outlineOrigin + checkboxSize};

    ImGui::PushID("Checkbox");
    ImGui::SetCursorScreenPos(checkboxRect.Min);
    bool pressed = ImGui::InvisibleButton(label, checkboxSize, ImGuiButtonFlags_MouseButtonLeft);
    ImGui::PopID();

    if (pressed) {
        value = !(value);
    }

    drawList->AddRect(outlineOrigin,
                      outlineOrigin + ImVec2(CHECKBOX_SIZE, CHECKBOX_SIZE),
                      outlineColor, 1.0f);


    ImVec2 offset = {CHECKBOX_PADDING, CHECKBOX_PADDING};
    if (value) {
        drawList->AddRectFilled(
            outlineOrigin + offset,
            outlineOrigin + ImVec2(CHECKBOX_SIZE, CHECKBOX_SIZE) - offset,
            outlineColor);
    }

    float textHeight = GetTextLineHeightWithSpacing();
    ImVec2 labelOrigin = {
        dst.Min.x, 
        (labelBelow ? 
            (checkboxRect.Max.y + labelOffset) : 
            (checkboxRect.Min.y - textHeight - labelOffset))
    };

    ImVec2 labelSize = {dst.GetWidth(), textHeight};
    RenderTextCentered(label, ImRect(labelOrigin, labelOrigin + labelSize));
    return pressed;
}

}  // namespace ImGui

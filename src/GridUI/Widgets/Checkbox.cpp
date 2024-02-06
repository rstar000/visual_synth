#include "GridUI/Colors.hpp"
#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "Checkbox.hpp"

#include "util.h"
#include "GridUI/Widgets/WidgetUtils.hpp"

namespace ImGui {

constexpr float CHECKBOX_SIZE = 20.0f;
constexpr float CHECKBOX_PADDING = 2.0f;

bool DrawCheckboxImpl(ColorScheme const& colors, const char* label, const ImRect& dst, bool* value, bool labelBelow, float labelOffset) 
{
    const ImVec2 outlineOrigin = {dst.Min.x + (dst.GetWidth() - CHECKBOX_SIZE) * 0.5f,
                                  dst.Min.y + (dst.GetHeight() - CHECKBOX_SIZE) * 0.5f};
    auto drawList = ImGui::GetWindowDrawList();

    ImU32 disabledColor = colors.nodeColors.titleBar.hovered;  //GetColorU32(ImGuiCol_FrameBg);
    ImU32 enabledColor = colors.selection.primary; //GetColorU32(ImGuiCol_FrameBgActive);

    ImVec2 checkboxSize = {CHECKBOX_SIZE, CHECKBOX_SIZE};
    ImRect checkboxRect = ImRect{outlineOrigin, outlineOrigin + checkboxSize};

    ImGui::PushID("Checkbox");
    ImGui::SetCursorScreenPos(checkboxRect.Min);
    bool pressed = ImGui::InvisibleButton(label, checkboxSize, ImGuiButtonFlags_MouseButtonLeft);
    ImGui::PopID();

    if (pressed) {
        *value = !(*value);
    }

    ImVec2 offset = {CHECKBOX_PADDING, CHECKBOX_PADDING};
    if (*value) {
        drawList->AddRect(outlineOrigin,
                        outlineOrigin + ImVec2(CHECKBOX_SIZE, CHECKBOX_SIZE),
                        enabledColor, 1.0f);

        drawList->AddRectFilled(
            outlineOrigin + offset,
            outlineOrigin + ImVec2(CHECKBOX_SIZE, CHECKBOX_SIZE) - offset,
            enabledColor);
    } else {
        drawList->AddRect(outlineOrigin,
                        outlineOrigin + ImVec2(CHECKBOX_SIZE, CHECKBOX_SIZE),
                        disabledColor, 1.0f);
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

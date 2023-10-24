#pragma once

#include "util.h"

namespace ImGui 
{
    void RenderTextCenteredImpl(const char* label, ImRect dst);
}

inline void RenderTextCentered(const char* label, ImRect dst)
{
    ImGui::RenderTextCenteredImpl(label, dst);
}

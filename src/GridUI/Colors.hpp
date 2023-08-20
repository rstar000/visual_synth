#pragma once
#include "imgui.h"

using ColorRGBA = ImU32;

struct ColorRGB {
  float r, g, b;
}; 

struct ColorHSL {
  float h, s, l;
};

ImU32 GenGrey(int x);

struct ColorScheme
{
    struct ColorStates {
        ColorRGBA normal;
        ColorRGBA hovered;
        ColorRGBA selected;
    };

    struct PinColors {
        ColorStates fill;
        ColorStates border;
    };

    struct GridColors {
        ColorRGBA background;
        ColorRGBA line;
        ColorRGBA lineAxis;
    };

    struct NodeColors {
        ColorStates titleBar;
        ColorRGBA background;
        ColorStates border;
    };

    struct WidgetColors {
        ColorRGBA primary;
        ColorRGBA secondary;
        ColorRGBA text;
    };

    PinColors pinColors;
    GridColors gridColors;
    NodeColors nodeColors;
    WidgetColors widgetColors;

    static ColorScheme GenerateDefault();
};

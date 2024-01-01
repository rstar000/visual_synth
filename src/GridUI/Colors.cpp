#include "Colors.hpp"

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cstdint>
#include <tuple>

uint8_t HexToU8(std::string hex) {
    unsigned long x = std::stoul(hex, nullptr, 16);
    return static_cast<uint8_t>(x);
}

ColorRGB ColorHexToRGB(std::string hex) {
    std::stringstream ss;
    uint8_t r, g, b;
    r = HexToU8(hex.substr(0, 2));
    g = HexToU8(hex.substr(2, 2));
    b = HexToU8(hex.substr(4, 2));
    std::cout << "Hex: " << hex << std::endl;
    std::cout << int(r) << ' ' << int(g) << ' ' << int(b) << std::endl;
    return ColorRGB{static_cast<float>(r), static_cast<float>(g), static_cast<float>(b)};
}

ImU32 GenGrey(int x)
{
    return IM_COL32(x, x, x, 255);
}

/*
 * Converts an RGB color value to HSL. Conversion formula
 * adapted from http://en.wikipedia.org/wiki/HSL_color_space.
 * Assumes r, g, and b are contained in the set [0, 255] and
 * returns HSL in the set [0, 1].
 */
ColorHSL rgb2hsl(ColorRGB rgb) {
    auto& [r, g, b] = rgb;
    ColorHSL result;

    r /= 255;
    g /= 255;
    b /= 255;

    float max = std::max(r, std::max(g, b));
    float min = std::min(r, std::min(g, b));

    result.h = result.s = result.l = (max + min) / 2;

    if (max == min) {
        result.h = result.s = 0;  // achromatic
    } else {
        float d = max - min;
        result.s = (result.l > 0.5) ? d / (2 - max - min) : d / (max + min);

        if (max == r) {
            result.h = (g - b) / d + (g < b ? 6 : 0);
        } else if (max == g) {
            result.h = (b - r) / d + 2;
        } else if (max == b) {
            result.h = (r - g) / d + 4;
        }

        result.h /= 6;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////

/*
 * Converts an HUE to r, g or b.
 * returns float in the set [0, 1].
 */
float hue2rgb(float p, float q, float t) {
    if (t < 0) t += 1;
    if (t > 1) t -= 1;
    if (t < 1. / 6) return p + (q - p) * 6 * t;
    if (t < 1. / 2) return q;
    if (t < 2. / 3) return p + (q - p) * (2. / 3 - t) * 6;

    return p;
}

////////////////////////////////////////////////////////////////////////

/*
 * Converts an HSL color value to RGB. Conversion formula
 * adapted from http://en.wikipedia.org/wiki/HSL_color_space.
 * Assumes h, s, and l are contained in the set [0, 1] and
 * returns RGB in the set [0, 255].
 */
ColorRGB hsl2rgb(ColorHSL hsl) {
    auto& [h, s, l] = hsl;
    ColorRGB result;

    if (0 == s) {
        result.r = result.g = result.b = l * 255;  // achromatic
    } else {
        float q = l < 0.5 ? l * (1 + s) : l + s - l * s;
        float p = 2 * l - q;
        result.r = hue2rgb(p, q, h + 1. / 3) * 255;
        result.g = hue2rgb(p, q, h) * 255;
        result.b = hue2rgb(p, q, h - 1. / 3) * 255;
    }

    return result;
}

std::vector<ColorRGB> GenPaletteRainbow() {
    std::vector<std::string> hexColors = {"f94144", "f3722c", "f8961e",
                                          "f9c74f", "90be6d", "43aa8b",
                                          "577590", "7d618f", "191B24"};
    std::vector<ColorRGB> colors(hexColors.size());
    std::transform(hexColors.begin(), hexColors.end(), colors.begin(),
                   ColorHexToRGB);
    return colors;
}

ColorRGBA RgbToRgba(ColorRGB color, uint8_t opacity = 255) {
    return IM_COL32(static_cast<uint8_t>(color.r), static_cast<uint8_t>(color.g), static_cast<uint8_t>(color.b), opacity);
}

ColorRGB AdjustBrightness(ColorRGB input, float delta)
{
    ColorHSL hsl = rgb2hsl(input);
    hsl.l = std::min(std::max(hsl.l + delta, 0.0f), 1.0f);
    return hsl2rgb(hsl);
}

ColorScheme ColorScheme::GenerateDefault() {
    const ColorRGB DARK_GREY_1 = ColorHexToRGB("5A5553");
    const ColorRGB DARK_GREY_2 = ColorHexToRGB("756E6C");
    const ColorRGB LIGHT_GREY_1 = ColorHexToRGB("938C8A");
    const ColorRGB LIGHT_GREY_2 = ColorHexToRGB("B1ACAA");

    const ColorRGB YELLOW_1 = ColorHexToRGB("F9C54E");
    const ColorRGB ORANGE_1 = ColorHexToRGB("F58B51");
    const ColorRGB RED_1 = ColorHexToRGB("FA6163");
    const ColorRGB GREEN_1 = ColorHexToRGB("90BE6D");
    const ColorRGB GREEN_2 = ColorHexToRGB("50B99A");
    const ColorRGB PURPLE_1 = ColorHexToRGB("7366A3");

    const ColorRGB DARKBLUE_1 = ColorHexToRGB("16161E");
    const ColorRGB DARKBLUE_2 = ColorHexToRGB("1A1B26");

    ColorScheme colors;
    auto palette = GenPaletteRainbow();
    colors.pinColors.fill = ColorScheme::ColorStates {
        .normal = RgbToRgba(DARK_GREY_1),
        .hovered =  RgbToRgba(ORANGE_1),
        .selected = RgbToRgba(RED_1)
    };

    colors.pinColors.border = ColorScheme::ColorStates {
        .normal   = RgbToRgba(AdjustBrightness(YELLOW_1, -0.3f)),
        .hovered  = RgbToRgba(AdjustBrightness(ORANGE_1, -0.3f)),
        .selected = RgbToRgba(AdjustBrightness(RED_1, -0.3f))
    };

    colors.gridColors = {
        .background = RgbToRgba(DARKBLUE_1),
        .line = GenGrey(30),
        .lineAxis = GenGrey(100)
    };

    colors.nodeColors.titleBar = {
        .normal   = RgbToRgba(DARK_GREY_2),
        .hovered  = RgbToRgba(LIGHT_GREY_2),
        .selected = RgbToRgba(RED_1),
    };

    colors.nodeColors.border = {
        .normal   = RgbToRgba(DARK_GREY_2),
        .hovered  = RgbToRgba(DARK_GREY_2),
        .selected = RgbToRgba(RED_1),
    };

    colors.nodeColors.background = RgbToRgba(DARKBLUE_2);

    colors.selection = ColorScheme::WidgetColors {
        .primary = RgbToRgba(RED_1),
        .secondary   = RgbToRgba(AdjustBrightness(RED_1, -0.3f)),
        .text   = GenGrey(220)
    };

    colors.range = ColorScheme::WidgetColors {
        .primary = RgbToRgba(GREEN_1),
        .secondary   = RgbToRgba(GREEN_2),
        .text   = GenGrey(220)
    };

    colors.display = ColorScheme::WidgetColors {
        .primary = RgbToRgba(PURPLE_1),
        .secondary   = RgbToRgba(AdjustBrightness(PURPLE_1, -0.3f)),
        .text   = GenGrey(220)
    };

    colors.hovered = ColorScheme::WidgetColors {
        .primary   = RgbToRgba(AdjustBrightness(YELLOW_1, 0.0f), 200),
        .secondary  = RgbToRgba(AdjustBrightness(ORANGE_1, 0.0f), 200),
        .text   = GenGrey(220)
    };

    colors.inactive = ColorScheme::WidgetColors {
        .primary   = RgbToRgba(AdjustBrightness(LIGHT_GREY_1, 0.0f)),
        .secondary  = RgbToRgba(AdjustBrightness(LIGHT_GREY_2, 0.0f)),
        .text   = GenGrey(220)
    };
    return colors;
}

ColorScheme ColorScheme::GenerateGruvbox() {
    const ColorRGB BG_0 = ColorHexToRGB("282828");
    const ColorRGB BG_1 = ColorHexToRGB("3C3836");
    const ColorRGB BG_2 = ColorHexToRGB("504945");
    const ColorRGB BG_3 = ColorHexToRGB("665C54");

    const ColorRGB FG_0 = ColorHexToRGB("FBF1C7");
    const ColorRGB FG_1 = ColorHexToRGB("EBDBB2");
    const ColorRGB FG_2 = ColorHexToRGB("D5C4A1");
    const ColorRGB FG_3 = ColorHexToRGB("BDAE93");

    const ColorRGB RED_1 = ColorHexToRGB("CC241D");
    const ColorRGB RED_2 = ColorHexToRGB("FB4934");
    const ColorRGB YELLOW_1 = ColorHexToRGB("D79921");
    const ColorRGB YELLOW_2 = ColorHexToRGB("FABD2F");
    const ColorRGB ORANGE_1 = ColorHexToRGB("D65D0E");
    const ColorRGB ORANGE_2 = ColorHexToRGB("FE8019");
    const ColorRGB GREEN_1 = ColorHexToRGB("98971A");
    const ColorRGB GREEN_2 = ColorHexToRGB("B8BB26");
    const ColorRGB BLUE_1 = ColorHexToRGB("458588");
    const ColorRGB BLUE_2 = ColorHexToRGB("83A598");
    const ColorRGB PURPLE_1 = ColorHexToRGB("B16286");
    const ColorRGB PURPLE_2 = ColorHexToRGB("D3869B");
    const ColorRGB AQUA_1 = ColorHexToRGB("689D6A");
    const ColorRGB AQUA_2 = ColorHexToRGB("8EC07C");


    ColorScheme colors;
    auto palette = GenPaletteRainbow();
    colors.pinColors.fill = ColorScheme::ColorStates {
        .normal = RgbToRgba(FG_3),
        .hovered =  RgbToRgba(ORANGE_1),
        .selected = RgbToRgba(RED_1)
    };

    colors.pinColors.border = ColorScheme::ColorStates {
        .normal   = RgbToRgba(YELLOW_2),
        .hovered  = RgbToRgba(ORANGE_2),
        .selected = RgbToRgba(RED_2)
    };

    colors.gridColors = {
        .background = RgbToRgba(BG_0),
        .line = RgbToRgba(BG_1),
        .lineAxis = RgbToRgba(BG_3)
    };

    colors.nodeColors.titleBar = {
        .normal   = RgbToRgba(BG_3),
        .hovered  = RgbToRgba(BG_3),
        .selected = RgbToRgba(RED_1),
    };

    colors.nodeColors.border = {
        .normal   = RgbToRgba(BG_3),
        .hovered  = RgbToRgba(BG_3),
        .selected = RgbToRgba(RED_1),
    };

    colors.nodeColors.background = RgbToRgba(BG_1);

    colors.selection = ColorScheme::WidgetColors {
        .primary = RgbToRgba(RED_1),
        .secondary   = RgbToRgba(RED_2),
        .background = RgbToRgba(BG_3),
        .text   = RgbToRgba(FG_0)
    };

    colors.range = ColorScheme::WidgetColors {
        .primary = RgbToRgba(GREEN_1),
        .secondary   = RgbToRgba(GREEN_2),
        .text   = GenGrey(220)
    };

    colors.display = ColorScheme::WidgetColors {
        .primary = RgbToRgba(PURPLE_1),
        .secondary   = RgbToRgba(PURPLE_2),
        .background = RgbToRgba(BG_2),
        .text   = RgbToRgba(FG_0)
    };

    colors.hovered = ColorScheme::WidgetColors {
        .primary   = RgbToRgba(YELLOW_2),
        .secondary  = RgbToRgba(ORANGE_2),
        .text   = GenGrey(220)
    };

    colors.inactive = ColorScheme::WidgetColors {
        .primary   = RgbToRgba(AdjustBrightness(PURPLE_1, 0.0f)),
        .secondary  = RgbToRgba(AdjustBrightness(PURPLE_2, 0.0f)),
        .text   = GenGrey(220)
    };
    return colors;
}

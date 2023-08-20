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
                                          "577590", "7d618f", "2f4550"};
    std::vector<ColorRGB> colors(hexColors.size());
    std::transform(hexColors.begin(), hexColors.end(), colors.begin(),
                   ColorHexToRGB);
    return colors;
}

ColorRGBA RgbToRgba(ColorRGB color) {
    return IM_COL32(static_cast<uint8_t>(color.r), static_cast<uint8_t>(color.g), static_cast<uint8_t>(color.b), 255);
}

ColorRGB AdjustBrightness(ColorRGB input, float delta)
{
    ColorHSL hsl = rgb2hsl(input);
    hsl.l = std::min(std::max(hsl.l + delta, 0.0f), 1.0f);
    return hsl2rgb(hsl);
}

ColorScheme ColorScheme::GenerateDefault() {
    ColorScheme colors;
    auto palette = GenPaletteRainbow();
    colors.pinColors.fill = ColorScheme::ColorStates {
        .normal = RgbToRgba(palette[3]),
        .hovered =  RgbToRgba(palette[2]),
        .selected = RgbToRgba(palette[0])
    };

    colors.pinColors.border = ColorScheme::ColorStates {
        .normal   = RgbToRgba(AdjustBrightness(palette[3], -0.3f)),
        .hovered  = RgbToRgba(AdjustBrightness(palette[2], -0.3f)),
        .selected = RgbToRgba(AdjustBrightness(palette[0], -0.3f))
    };

    colors.gridColors = {
        .background = GenGrey(0),
        .line = GenGrey(30),
        .lineAxis = GenGrey(100)
    };

    colors.nodeColors.titleBar = {
        .normal   = RgbToRgba(palette[6]),
        .hovered  = RgbToRgba(palette[7]),
        .selected = RgbToRgba(palette[0]),
    };

    colors.nodeColors.border = {
        .normal   = RgbToRgba(palette[6]),
        .hovered  = RgbToRgba(palette[7]),
        .selected = RgbToRgba(palette[0]),
    };

    colors.nodeColors.background = RgbToRgba(palette[8]);

    colors.widgetColors = ColorScheme::WidgetColors {
        .primary = RgbToRgba(palette[4]),
        .secondary   = RgbToRgba(AdjustBrightness(palette[4], -0.3f)),
        .text   = GenGrey(220)
    };
    return colors;
}
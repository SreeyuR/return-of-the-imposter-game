#include "color.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * Converts an HSV color to RGB.
 * Source: https://www.rapidtables.com/convert/color/hsv-to-rgb.html
 */
rgba_color_t hsv_to_rgba(hsv_color_t hsv) {
    float h = hsv.h;
    float s = hsv.s;
    float v = hsv.v;
    assert(h >= 0 && h < 360 && s >= 0 && s <= 1 && v >= 0 && v <= 1);
    float c = v * s;
    float x = c * (1 - fabs(fmod((h / 60), 2) - 1));
    float m = v - c;
    float r1, g1, b1;
    if (h < 60) {
        r1 = c;
        g1 = x;
        b1 = 0;
    } else if (h < 120) {
        r1 = x;
        g1 = c;
        b1 = 0;
    } else if (h < 180) {
        r1 = 0;
        g1 = c;
        b1 = x;
    } else if (h < 240) {
        r1 = 0;
        g1 = x;
        b1 = c;
    } else if (h < 300) {
        r1 = x;
        g1 = 0;
        b1 = c;
    } else {
        r1 = c;
        g1 = 0;
        b1 = x;
    }
    float r = r1 + m;
    float g = g1 + m;
    float b = b1 + m;
    float a = 1;
    rgba_color_t result = {.r = r, .g = g, .b = b, .a = a};
    return result;
}

rgba_color_t hex_to_rgba(uint32_t hex) {
    float r = (float)((hex >> 4) & 0xff) / 255;
    float g = (float)((hex >> 2) & 0xff) / 255;
    float b = (float)(hex & 0xff) / 255;
    float a = (float)((hex >> 6) & 0xff) / 255;
    rgba_color_t result = {.r = r, .g = g, .b = b, .a = a};
    return result;
}

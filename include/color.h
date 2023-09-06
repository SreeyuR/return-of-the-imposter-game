#ifndef __COLOR_H__
#define __COLOR_H__

#include <stdint.h>

/**
 * A color to display on the screen.
 * The color is represented by its red, green, and blue components.
 * Each component must be between 0 (black) and 1 (white).
 */
typedef struct {
    float r;
    float g;
    float b;
    float a;
} rgba_color_t;

/**
 * A color to display on the screen.
 * The color is represented by its hue, saturation, and value components.
 */
typedef struct {
    float h;
    float s;
    float v;
} hsv_color_t;

#define COLOR_BLACK ((rgba_color_t){0, 0, 0, 1})
#define COLOR_WHITE ((rgba_color_t){1, 1, 1, 1})
#define COLOR_RED ((rgba_color_t){1, 0, 0, 1})
#define COLOR_GREEN ((rgba_color_t){0, 1, 0, 1})
#define COLOR_BLUE ((rgba_color_t){0, 0, 1, 1})
#define COLOR_YELLOW ((rgba_color_t){1, 1, 0, 1})
#define COLOR_MAGENTA ((rgba_color_t){1, 0, 1, 1})
#define COLOR_CYAN ((rgba_color_t){0, 1, 1, 1})
#define COLOR_ORANGE ((rgba_color_t){1, 0.5, 0, 1})
#define COLOR_TRANSPARENT ((rgba_color_t){0, 0, 0, 0})

rgba_color_t hsv_to_rgba(hsv_color_t);

rgba_color_t hex_to_rgba(uint32_t hex);

#endif // #ifndef __COLOR_H__

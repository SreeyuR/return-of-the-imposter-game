#ifndef __TEXTURE_WRAPPER_H__
#define __TEXTURE_WRAPPER_H__

#include "bounding_box.h"
#include "color.h"
#include "vector.h"
#include <stddef.h>

typedef struct texture_wrapper texture_wrapper_t;

/**
 * The possible options for rendering a texture.
 *
 * STRETCH_TO_FIT - stretches the texture to fit the target area, potentially
 *      causing distortion.
 * PRESERVE_ASPECT_RATIO_AND_EXPAND - preserves the
 *      aspect ratio of the texture while expanding them to fit the target area,
 *      potentially resulting in empty space. This will be the default render
 *      option, as it is the most conventional way of displaying text.
 * PRESERVE_SCALE_AND_TILE - preserves the scale of the texture. The original
 *      dimensions of the source texture are preserved, and multiple copies of
 * it are tiled to fill the target area.
 */
typedef enum {
    STRETCH_TO_FIT,
    PRESERVE_ASPECT_RATIO_AND_EXPAND,
    PRESERVE_SCALE_AND_TILE
} render_option_t;

/**
 * Initializes a texture_wrapper_t struct. Note: the img_texture and
 * text_texture fields are default set to NULL.
 *
 * @param scene_bbox the bounding box that the texture will be enclosed within.
 * @return a pointer to the newly created texture_wrapper_t struct
 */
texture_wrapper_t *texture_wrapper_init(bounding_box_t scene_bbox);

void texture_wrapper_set_flip(texture_wrapper_t *texture_wrapper,
                              bool horizontal_flip, bool vertical_flip);

void texture_wrapper_set_img_texture(texture_wrapper_t *texture_wrapper,
                                     const char *img_file,
                                     render_option_t img_render_option);

void texture_wrapper_set_text_texture(texture_wrapper_t *texture_wrapper,
                                      char *text, const char *font_path,
                                      size_t font_size, rgba_color_t text_color,
                                      render_option_t text_render_option);

void texture_wrapper_set_visibility(texture_wrapper_t *texture_wrapper,
                                    bool visibility);

void texture_translate(texture_wrapper_t *texture, vector_t translation);

void texture_wrapper_free(texture_wrapper_t *texture);

#endif // #ifndef __TEXTURE_WRAPPER_H__

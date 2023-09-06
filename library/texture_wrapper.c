#include "texture_wrapper.h"
#include <stdlib.h>

typedef struct texture_wrapper {

} texture_wrapper_t;

texture_wrapper_t *texture_wrapper_init(bounding_box_t scene_bbox) {
    return NULL;
}

void texture_wrapper_set_flip(texture_wrapper_t *texture_wrapper,
                              bool horizontal_flip, bool vertical_flip) {
    return;
}

void texture_wrapper_set_img_texture(texture_wrapper_t *texture_wrapper,
                                     const char *img_file,
                                     render_option_t img_render_option) {
    return;
}

void texture_wrapper_set_text_texture(texture_wrapper_t *texture_wrapper,
                                      char *text, const char *font_path,
                                      size_t font_size, rgba_color_t text_color,
                                      render_option_t text_render_option) {
    return;
}

void texture_wrapper_set_visibility(texture_wrapper_t *texture_wrapper,
                                    bool visibility) {
    return;
}

void texture_translate(texture_wrapper_t *texture, vector_t translation) {
    return;
}

void texture_wrapper_free(texture_wrapper_t *texture) {
    return;
}
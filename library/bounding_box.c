#include "bounding_box.h"

bounding_box_t bounding_box_translate(bounding_box_t bbox,
                                      vector_t translation) {
    return (bounding_box_t){
        .min_x = bbox.min_x + translation.x,
        .min_y = bbox.min_y + translation.y,
        .max_x = bbox.max_x + translation.x,
        .max_y = bbox.max_y + translation.y,
    };
}

vector_t bounding_box_center(bounding_box_t bbox) {
    return (vector_t){.x = (bbox.min_x + bbox.max_x) / 2,
                      .y = (bbox.min_y + bbox.max_y) / 2};
}

bool bounding_box_contains_point(bounding_box_t bbox, vector_t point) {
    return point.x > bbox.min_x && point.x < bbox.max_x &&
           point.y > bbox.min_y && point.y < bbox.max_y;
}

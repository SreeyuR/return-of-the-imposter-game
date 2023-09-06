#ifndef __BOUNDING_BOX_H__
#define __BOUNDING_BOX_H__

#include "vector.h"
#include <math.h>
#include <stdbool.h>

#define INFINITE_BBOX ((bounding_box_t){.min_x = -INFINITY, .min_y = -INFINITY, .max_x = INFINITY, .max_y = INFINITY})

typedef struct bounding_box {
    double min_x;
    double min_y;
    double max_x;
    double max_y;
} bounding_box_t;

bounding_box_t bounding_box_translate(bounding_box_t bbox, vector_t translation);

vector_t bounding_box_center(bounding_box_t bbox);

bool bounding_box_contains_point(bounding_box_t bbox, vector_t point);

#endif // #ifndef __BOUNDING_BOX_H__

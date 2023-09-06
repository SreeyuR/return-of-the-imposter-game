#include "collision.h"
#include "list.h"
#include "utils.h"
#include "vector.h"
#include <float.h>
#include <math.h>
#include <stdlib.h>

list_t *get_perpendicular_axes(list_t *shape) {
    list_t *perpendicular_axes = list_init(list_size(shape), free);
    for (size_t v1 = 0; v1 < list_size(shape); v1++) {
        size_t v2 = (v1 + 1) % list_size(shape);
        vector_t *first_vertex = list_get(shape, v1);
        vector_t *second_vertex = list_get(shape, v2);
        vector_t edge = vec_subtract(*second_vertex, *first_vertex);
        vector_t *perpendicular_axis = malloc(sizeof(vector_t));
        *perpendicular_axis = vec_direction(vec_rotate(edge, PI / 2));
        list_add(perpendicular_axes, perpendicular_axis);
    }
    return perpendicular_axes;
}

double get_projection_min(list_t *shape, vector_t axis) {
    double min = INFINITY;
    for (size_t i = 0; i < list_size(shape); i++) {
        double projection = vec_dot(*(vector_t *)list_get(shape, i), axis);
        if (projection < min) {
            min = projection;
        }
    }
    return min;
}

double get_projection_max(list_t *shape, vector_t axis) {
    double max = -INFINITY;
    for (size_t i = 0; i < list_size(shape); i++) {
        double projection = vec_dot(*(vector_t *)list_get(shape, i), axis);
        if (projection > max) {
            max = projection;
        }
    }
    return max;
}

collision_info_t find_collision(list_t *shape1, list_t *shape2) {
    list_t *perpendicular_axes1 = get_perpendicular_axes(shape1);
    list_t *perpendicular_axes2 = get_perpendicular_axes(shape2);
    list_t *perpendicular_axes =
        list_append(perpendicular_axes1, perpendicular_axes2);
    vector_t min_overlap_axis;
    double min_overlap = INFINITY;
    collision_status_t min_overlap_collision_status;
    for (size_t i = 0; i < list_size(perpendicular_axes); i++) {
        vector_t axis = *(vector_t *)list_get(perpendicular_axes, i);
        double min_shape1 = get_projection_min(shape1, axis);
        double max_shape1 = get_projection_max(shape1, axis);
        double min_shape2 = get_projection_min(shape2, axis);
        double max_shape2 = get_projection_max(shape2, axis);
        double overlap =
            segment_overlap(min_shape1, max_shape1, min_shape2, max_shape2);
        if (overlap < min_overlap) {
            // Ensure that the axis goes from shape1 to shape2
            if ((min_shape2 + max_shape2) < (min_shape1 + max_shape1)) {
                axis = vec_negate(axis);
            }

            min_overlap_axis = axis;
            min_overlap = overlap;

            if (min_overlap == 0) {
                min_overlap_collision_status = NO_COLLISION;
                break;
            } else if ((min_shape1 <= min_shape2 && max_shape1 >= max_shape2) ||
                       (min_shape2 <= min_shape1 && max_shape2 >= max_shape1)) {
                min_overlap_collision_status = FULL_COLLISION;
            } else {
                min_overlap_collision_status = PARTIAL_COLLISION;
            }
        }
    }
    list_free(perpendicular_axes1);
    list_free(perpendicular_axes2);
    list_free(perpendicular_axes);
    collision_info_t result = {.collided = min_overlap_collision_status,
                               .axis = min_overlap_axis, .overlap = min_overlap};
    return result;
}

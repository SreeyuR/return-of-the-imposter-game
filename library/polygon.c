#include "polygon.h"
#include "utils.h"
#include "vector.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

bounding_box_t polygon_get_bounding_box(list_t *polygon) {
    double min_x = INFINITY;
    double min_y = INFINITY;
    double max_x = -INFINITY;
    double max_y = -INFINITY;
    for (size_t i = 0; i < list_size(polygon); i++) {
        vector_t *vertex = list_get(polygon, i);
        if (vertex->x < min_x) {
            min_x = vertex->x;
        }
        if (vertex->y < min_y) {
            min_y = vertex->y;
        }
        if (vertex->x > max_x) {
            max_x = vertex->x;
        }
        if (vertex->y > max_y) {
            max_y = vertex->y;
        }
    }
    bounding_box_t bounding_box = {
        .min_x = min_x, .min_y = min_y, .max_x = max_x, .max_y = max_y};
    return bounding_box;
}

double polygon_area(list_t *polygon) {
    double area = 0;
    size_t num_verts = list_size(polygon);
    for (size_t i = 0; i < num_verts; i++) {
        vector_t *vec1 = list_get(polygon, i);
        vector_t *vec2 = list_get(polygon, (i + 1) % num_verts);
        area += vec_cross(*vec1, *vec2) / 2;
    }
    return area;
}

vector_t polygon_centroid(list_t *polygon) {
    double centroid_x = 0;
    double centroid_y = 0;
    size_t num_verts = list_size(polygon);
    double area = polygon_area(polygon);
    for (size_t i = 0; i < num_verts; i++) {
        vector_t *vec1 = list_get(polygon, i);
        vector_t *vec2 = list_get(polygon, (i + 1) % num_verts);
        centroid_x += (vec1->x + vec2->x) * vec_cross(*vec1, *vec2);
        centroid_y += (vec1->y + vec2->y) * vec_cross(*vec1, *vec2);
    }
    centroid_x /= (6 * area);
    centroid_y /= (6 * area);
    vector_t result = {.x = centroid_x, .y = centroid_y};
    return result;
}

void polygon_translate(list_t *polygon, vector_t translation) {
    size_t num_verts = list_size(polygon);
    for (size_t i = 0; i < num_verts; i++) {
        vector_t *vec = list_get(polygon, i);
        *vec = vec_add(*vec, translation);
    }
}

void polygon_rotate(list_t *polygon, double angle, vector_t point) {
    size_t num_verts = list_size(polygon);
    polygon_translate(polygon, vec_negate(point));
    for (size_t i = 0; i < num_verts; i++) {
        vector_t *vec = list_get(polygon, i);
        *vec = vec_rotate(*vec, angle);
    }
    polygon_translate(polygon, point);
}

void move_anchor_to_current_center(list_t *polygon, anchor_option_t anchor) {
    bounding_box_t bbox = polygon_get_bounding_box(polygon);
    vector_t centroid = polygon_centroid(polygon);
    double x_translation = 0;
    double y_translation = 0;
    if (anchor.x_anchor == ANCHOR_MIN) {
        x_translation = centroid.x - bbox.min_x;
    } else if (anchor.x_anchor == ANCHOR_MAX) {
        x_translation = centroid.x - bbox.max_x;
    } else {
        assert(anchor.x_anchor == ANCHOR_CENTER);
    }
    if (anchor.y_anchor == ANCHOR_MIN) {
        y_translation = centroid.y - bbox.min_y;
    } else if (anchor.y_anchor == ANCHOR_MAX) {
        y_translation = centroid.y - bbox.max_y;
    } else {
        assert(anchor.y_anchor == ANCHOR_CENTER);
    }
    vector_t translation = {.x = x_translation, .y = y_translation};
    polygon_translate(polygon, translation);
}

list_t *initialize_star(vector_t center, size_t num_arms, double circumradius,
                        double inradius) {
    assert(circumradius >= inradius);
    size_t num_verts = num_arms * 2;
    list_t *star = list_init(num_verts, free);
    for (size_t i = 0; i < num_verts; i++) {
        double theta = (2 * PI / num_verts) * i;
        double radius = (i % 2 == 0) ? circumradius : inradius;
        vector_t *vertex = vec_init(radius * cos(theta), radius * sin(theta));
        list_add(star, vertex);
    }
    polygon_translate(star, center);
    return star;
}

list_t *initialize_star_anchored(anchor_option_t anchor, vector_t pos,
                                 size_t num_arms, double circumradius,
                                 double inradius) {
    list_t *result = initialize_star(pos, num_arms, circumradius, inradius);
    move_anchor_to_current_center(result, anchor);
    return result;
}

list_t *initialize_regular_polygon(vector_t center, double circumradius,
                                   size_t num_verts) {
    assert(circumradius > 0);
    list_t *shape = list_init(num_verts, free);
    for (size_t i = 0; i < num_verts; i++) {
        double theta = (2 * PI / num_verts) * i;
        vector_t *vertex =
            vec_init(circumradius * cos(theta), circumradius * sin(theta));
        list_add(shape, vertex);
    }
    polygon_translate(shape, center);
    return shape;
}

list_t *initialize_regular_polygon_anchored(anchor_option_t anchor,
                                            vector_t pos, double circumradius,
                                            size_t num_verts) {
    list_t *result = initialize_regular_polygon(pos, num_verts, circumradius);
    move_anchor_to_current_center(result, anchor);
    return result;
}

list_t *initialize_pacman(vector_t center, double mouth_angle,
                          double face_radius, size_t num_segments_pacman_back) {
    assert(face_radius > 0);
    list_t *shape = list_init(num_segments_pacman_back + 2, free);
    double initial_angle = mouth_angle / 2;
    // vertex at center of mouth
    vector_t *mouth_center = malloc(sizeof(vector_t));
    *(mouth_center) = VEC_ZERO;
    list_add(shape, mouth_center);
    for (size_t i = 0; i <= num_segments_pacman_back; i++) {
        double theta = initial_angle +
                       i * ((2 * PI - mouth_angle) / num_segments_pacman_back);
        vector_t *vertex =
            vec_init(face_radius * cos(theta), face_radius * sin(theta));
        list_add(shape, vertex);
    }
    /**
     * we want the centroid of the pacman (not the center of the circle used to
     * construct the pacman) to be positioned at `center`
     */
    polygon_translate(shape, vec_negate(polygon_centroid(shape)));
    polygon_translate(shape, center);
    return shape;
}

list_t *initialize_rectangle(double min_x, double min_y, double max_x,
                             double max_y) {
    assert(min_x <= max_x && min_y <= max_y);
    list_t *shape = list_init(4, free);
    list_add(shape, vec_init(min_x, min_y));
    list_add(shape, vec_init(max_x, min_y));
    list_add(shape, vec_init(max_x, max_y));
    list_add(shape, vec_init(min_x, max_y));
    return shape;
}

list_t *initialize_rectangle_centered(vector_t center, double width,
                                      double height) {
    assert(width >= 0 && height >= 0);
    double min_x = center.x - width / 2;
    double max_x = center.x + width / 2;
    double min_y = center.y - height / 2;
    double max_y = center.y + height / 2;
    return initialize_rectangle(min_x, min_y, max_x, max_y);
}

list_t *initialize_rectangle_anchored(anchor_option_t anchor, vector_t pos,
                                      double width, double height) {
    list_t *result = initialize_rectangle_centered(pos, width, height);
    move_anchor_to_current_center(result, anchor);
    return result;
}

list_t *initialize_rectangle_rotated(vector_t pos1, vector_t pos2,
                                     double width) {
    vector_t res = vec_subtract(pos2, pos1);
    double res_magnitude = vec_magnitude(res);
    // create rectangle that lies along x-axis and has left endpoint at origin
    list_t *shape =
        initialize_rectangle(0, -width / 2, res_magnitude, width / 2);

    // rotate that rectangle along the x-axis
    double theta = atan2(res.y, res.x);
    polygon_rotate(shape, theta, VEC_ZERO);

    // translate rectangle so that it becomes "anchored" between pos1 and pos2
    polygon_translate(shape, pos1);
    return shape;
}

list_t *initialize_ellipse(vector_t center, double width, double height,
                           size_t num_verts) {
    assert(width > 0 && height > 0);
    list_t *shape = list_init(num_verts, free);
    for (size_t i = 0; i < num_verts; i++) {
        double theta = (2 * PI / num_verts) * i;
        double x = (width / 2) * cos(theta);
        double y = (height / 2) * sin(theta);
        list_add(shape, vec_init(x, y));
    }
    polygon_translate(shape, center);
    return shape;
}

list_t *initialize_ellipse_anchored(anchor_option_t anchor, vector_t pos,
                                    double width, double height,
                                    size_t num_verts) {
    list_t *result = initialize_rectangle_centered(pos, width, height);
    move_anchor_to_current_center(result, anchor);
    return result;
}

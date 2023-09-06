#include "vector.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>

const vector_t VEC_ZERO = {.x = 0, .y = 0};

vector_t vec_add(vector_t v1, vector_t v2) {
    vector_t res = {.x = v1.x + v2.x, .y = v1.y + v2.y};
    return res;
}

vector_t vec_subtract(vector_t v1, vector_t v2) {
    return vec_add(v1, vec_negate(v2));
}

vector_t vec_negate(vector_t v) {
    return vec_multiply(-1, v);
}

vector_t vec_multiply(double scalar, vector_t v) {
    vector_t res = {.x = scalar * v.x, .y = scalar * v.y};
    return res;
}

double vec_dot(vector_t v1, vector_t v2) {
    return v1.x * v2.x + v1.y * v2.y;
}

double vec_cross(vector_t v1, vector_t v2) {
    return v1.x * v2.y - v1.y * v2.x;
}

double vec_magnitude(vector_t v) {
    return sqrt(vec_dot(v, v));
}

vector_t vec_direction(vector_t v) {
    if (vec_magnitude(v) == 0) {
        return VEC_ZERO;
    }
    return vec_multiply(1 / (vec_magnitude(v)), v);
}

double vec_distance(vector_t v1, vector_t v2) {
    return vec_magnitude(vec_subtract(v1, v2));
}

vector_t vec_rotate(vector_t v, double angle) {
    vector_t res = {.x = v.x * cos(angle) - v.y * sin(angle),
                    .y = v.x * sin(angle) + v.y * cos(angle)};
    return res;
}

vector_t *vec_init(double x, double y) {
    vector_t *res = malloc(sizeof(vector_t));
    assert(res);
    res->x = x;
    res->y = y;
    return res;
}

vector_t *vec_copy(vector_t *v) {
    vector_t *res = malloc(sizeof(vector_t));
    assert(res);
    res->x = v->x;
    res->y = v->y;
    return res;
}

#include "body.h"
#include "collision.h"
#include "polygon.h"
#include "sdl_wrapper.h"
#include "utils.h"
#include <assert.h>
#include <float.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct body {
    list_t *shape;
    double mass;
    rgba_color_t color;
    texture_wrapper_t *texture;
    vector_t velocity;
    vector_t acceleration;
    double orientation;
    vector_t centroid;
    double angular_velocity;
    vector_t net_force;
    vector_t net_impulse;
    bool is_marked_for_removal;
    void *info;
    free_func_t info_freer;
} body_t;

body_t *body_init(list_t *shape, double mass, rgba_color_t color) {
    return body_init_with_info(shape, mass, color, NULL, NULL);
}

body_t *body_init_with_info(list_t *shape, double mass, rgba_color_t color,
                            void *info, free_func_t info_freer) {
    body_t *body = malloc(sizeof(body_t));
    assert(body);
    bounding_box_t bbox = polygon_get_bounding_box(shape);
    *(body) = (body_t){.shape = shape,
                       .mass = mass,
                       .color = color,
                       .texture = texture_wrapper_init(bbox),
                       .velocity = VEC_ZERO,
                       .acceleration = VEC_ZERO,
                       .orientation = 0,
                       .centroid = polygon_centroid(shape),
                       .angular_velocity = 0,
                       .net_force = VEC_ZERO,
                       .net_impulse = VEC_ZERO,
                       .is_marked_for_removal = false,
                       .info = info,
                       .info_freer = info_freer};
    return body;
}

void body_free(body_t *body) {
    list_free(body->shape);
    if (body->info_freer) {
        body->info_freer(body->info);
    }
    if (body->texture) {
        texture_wrapper_free(body->texture);
    }
    free(body);
}

list_t *body_get_shape(body_t *body) {
    return list_copy(body->shape, (copy_func_t)vec_copy);
}

vector_t body_get_centroid(body_t *body) {
    return body->centroid;
}

vector_t body_get_velocity(body_t *body) {
    return body->velocity;
}

double body_get_mass(body_t *body) {
    return body->mass;
}

rgba_color_t body_get_color(body_t *body) {
    return body->color;
}

texture_wrapper_t *body_get_texture(body_t *body) {
    return body->texture;
}

bounding_box_t body_get_bounding_box(body_t *body) {
    return polygon_get_bounding_box(body->shape);
}

vector_t body_get_acceleration(body_t *body) {
    return body->acceleration;
}

void body_translate(body_t *body, vector_t translation) {
    polygon_translate(body->shape, translation);
    body->centroid = vec_add(body->centroid, translation);
    if (body->texture) {
        texture_translate(body->texture, translation);
    }
}

void body_rotate(body_t *body, double angle) {
    polygon_rotate(body->shape, angle, body->centroid);
    body->orientation += angle;
}

void *body_get_info(body_t *body) {
    return body->info;
}

void body_set_texture_flip(body_t *body, bool horizontal_flip, bool vertical_flip) {
    texture_wrapper_set_flip(body->texture, horizontal_flip, vertical_flip);
}

void body_set_img_texture(body_t *body, const char *img_file, render_option_t img_render_option) {
    texture_wrapper_set_img_texture(body->texture, img_file, img_render_option);
}

void body_set_text_texture(body_t *body, char *text,
                      const char *font_path, size_t font_size,
                      rgba_color_t text_color, render_option_t text_render_option) {
    texture_wrapper_set_text_texture(body->texture, text, font_path,
                                         font_size, text_color, text_render_option);
}

void body_set_visibility(body_t *body, bool visibility) {
    texture_wrapper_set_visibility(body->texture, visibility);
}

void body_set_centroid(body_t *body, vector_t x) {
    vector_t displacement = vec_subtract(x, body->centroid);
    body_translate(body, displacement);
}

void body_set_velocity(body_t *body, vector_t v) {
    body->velocity = v;
}

void body_set_mass(body_t *body, double mass) {
    body->mass = mass;
}

void body_set_color(body_t *body, rgba_color_t color) {
    body->color = color;
}

void body_set_angular_velocity(body_t *body, double angular_velocity) {
    body->angular_velocity = angular_velocity;
}

void body_set_rotation(body_t *body, double angle) {
    double d_theta = angle - body->orientation;
    body_rotate(body, d_theta);
}

void body_add_force(body_t *body, vector_t force) {
    body->net_force = vec_add(body->net_force, force);
}

void body_add_impulse(body_t *body, vector_t impulse) {
    body->net_impulse = vec_add(body->net_impulse, impulse);
}

/**
 * Helper function.
 * Rotates all vertices in a body based on its velocity
 * during a duration of time.
 * Note: mutates the original body.
 *
 * @param body a pointer to body that we want to rotate
 * @param dt the time that has passed since the body was last
 *               rotated.
 */
void update_rotation(body_t *body, double dt) {
    body_rotate(body, dt * body->angular_velocity);
}

/**
 * Helper function.
 * Translates all vertices in a body based on its velocity
 * during a duration of time.
 * Note: mutates the original body.
 *
 * @param body a pointer to body that we want to translate
 * @param dt the time that has passed since the body was last
 *               translated.
 */
void update_translation(body_t *body, double dt) {
    vector_t next_velocity = body->velocity;
    if ((body->mass != 0) && (body->mass != INFINITY)) {
        next_velocity = vec_add(next_velocity,
                                vec_multiply(dt / body->mass, body->net_force));
        next_velocity = vec_add(
            next_velocity, vec_multiply(1 / body->mass, body->net_impulse));
    }
    body->net_force = VEC_ZERO;
    body->net_impulse = VEC_ZERO;
    body_translate(
        body, vec_multiply(dt / 2, vec_add(body->velocity, next_velocity)));
    body->velocity = next_velocity;
}

void body_tick(body_t *body, double dt) {
    vector_t old_velocity = body_get_velocity(body);
    update_translation(body, dt);
    update_rotation(body, dt);
    vector_t new_velocity = body_get_velocity(body);
    vector_t accel = vec_multiply(1/dt, vec_subtract(new_velocity, old_velocity));
    body->acceleration = accel;
}

body_t *body_copy(body_t *body) {
    body_t *result = malloc(sizeof(body_t));
    assert(result);
    result->shape = list_copy(body->shape, (copy_func_t)vec_copy);
    result->mass = body->mass;
    result->color = body->color;
    result->velocity = body->velocity;
    result->orientation = body->orientation;
    result->angular_velocity = body->angular_velocity;
    result->net_force = body->net_force;
    result->net_impulse = body->net_impulse;
    result->info = NULL;
    result->info_freer = NULL;
    return result;
}

list_t *future_body_helper(body_t *body, double dt, bool translate,
                           bool rotate) {
    body_t *future_body = body_copy(body);
    if (translate) {
        update_translation(future_body, dt);
    }
    if (rotate) {
        update_rotation(future_body, dt);
    }
    list_t *result = body_get_shape(future_body);
    body_free(future_body);
    return result;
}

list_t *future_body_trans_rot(body_t *body, double dt) {
    return future_body_helper(body, dt, true, true);
}

list_t *future_body_translational(body_t *body, double dt) {
    return future_body_helper(body, dt, true, false);
}

list_t *future_body_rotational(body_t *body, double dt) {
    return future_body_helper(body, dt, false, true);
}

collision_info_t detect_body_collision(body_t *body1, body_t *body2) {
    return find_collision(body1->shape, body2->shape);
}

void body_remove(body_t *body) {
    body->is_marked_for_removal = true;
}

bool body_is_removed(body_t *body) {
    return body->is_marked_for_removal;
}

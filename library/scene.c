#include "scene.h"
#include "polygon.h"
#include "utils.h"
#include <assert.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>

// The initial capacity of the list that stores the bodies in the scene
const size_t DEFAULT_BODY_CAPACITY = 10;
const size_t DEFAULT_FORCE_CAPACITY = 15;

// The number of times we need to check a collision before we are sure
// that it is impossible for the body to go anywhere
const size_t DETECT_COLLISION_NUM_TRIES = 4;

const double BODY_SEGMENT_WIDTH = 0.01;

typedef struct force_creator_wrapper {
    force_creator_t forcer;
    void *aux;
    free_func_t freer;
    list_t *bodies;
    bool is_post_tick;
} force_creator_wrapper_t;

typedef struct scene {
    list_t *bodies;
    list_t *forces;
} scene_t;

void force_creator_wrapper_free(force_creator_wrapper_t *wrapper) {
    if (wrapper->freer) {
        wrapper->freer(wrapper->aux);
    }
    list_free(wrapper->bodies);
    free(wrapper);
}

scene_t *scene_init(void) {
    scene_t *scene = malloc(sizeof(scene_t));
    assert(scene);
    scene->bodies = list_init(DEFAULT_BODY_CAPACITY, (free_func_t)body_free);
    scene->forces = list_init(DEFAULT_FORCE_CAPACITY,
                              (free_func_t)force_creator_wrapper_free);
    return scene;
}

void scene_free(scene_t *scene) {
    list_free(scene->bodies);
    list_free(scene->forces);
    free(scene);
}

size_t scene_bodies(scene_t *scene) {
    return list_size(scene->bodies);
}

body_t *scene_get_body(scene_t *scene, size_t index) {
    return list_get(scene->bodies, index);
}

void scene_add_body(scene_t *scene, body_t *body) {
    list_add(scene->bodies, body);
}

void scene_remove_body(scene_t *scene, size_t index) {
    body_remove(list_get(scene->bodies, index));
}

void scene_clear(scene_t *scene) {
    list_clear(scene->bodies);
    list_clear(scene->forces);
}

void scene_add_force_creator(scene_t *scene, force_creator_t forcer, void *aux,
                             free_func_t freer) {
    list_t *bodies = list_init(0, NULL);
    scene_add_bodies_force_creator(scene, forcer, aux, bodies, freer);
}

void scene_add_bodies_force_creator(scene_t *scene, force_creator_t forcer,
                                    void *aux, list_t *bodies,
                                    free_func_t freer) {
    scene_add_bodies_generic_force_creator(scene, forcer, false, aux, bodies,
                                           freer);
}

void scene_add_bodies_generic_force_creator(scene_t *scene,
                                            force_creator_t forcer,
                                            bool is_post_tick, void *aux,
                                            list_t *bodies, free_func_t freer) {
    force_creator_wrapper_t *wrapper = malloc(sizeof(force_creator_wrapper_t));
    wrapper->forcer = forcer;
    wrapper->aux = aux;
    wrapper->freer = freer;
    wrapper->bodies = bodies;
    wrapper->is_post_tick = is_post_tick;
    list_add(scene->forces, wrapper);
}

bool scene_detect_line_of_sight(scene_t *scene, body_t *body1, body_t *body2, body_predicate_t opaqueness_predicate) {
    /**
     * Initializes a line segment body that extends from `body2` to `body1`.
     * The line segment has an infinitesimal width to ensure that the collision
     * axis is detected as parallel to the segment instead of perpendicular.
     */
    vector_t c1 = body_get_centroid(body1);
    vector_t c2 = body_get_centroid(body2);
    list_t *shape = initialize_rectangle_rotated(c1, c2, BODY_SEGMENT_WIDTH);
    body_t *body_segment = body_init(shape, 0, COLOR_WHITE);

    for (size_t i = 0; i < scene_bodies(scene); i++) {
        body_t *middle_body = scene_get_body(scene, i);
        if ((middle_body == body1) || (middle_body == body2)) {
            continue;
        }
        if ((!opaqueness_predicate || opaqueness_predicate(middle_body)) && detect_body_collision(body_segment, middle_body).collided) {
            body_free(body_segment);
            return false;
        }
    }
    body_free(body_segment);
    return true;
}

void scene_tick(scene_t *scene, double dt) {
    // force application (pre-tick)
    for (size_t i = 0; i < list_size(scene->forces); i++) {
        force_creator_wrapper_t *wrapper = list_get(scene->forces, i);
        if (!wrapper->is_post_tick) {
            wrapper->forcer(wrapper->aux);
        }
    }
    // force removal. Note that this has to be in a separate loop since
    // force application could mark some bodies for removal.
    for (size_t i = 0; i < list_size(scene->forces); i++) {
        force_creator_wrapper_t *wrapper = list_get(scene->forces, i);
        for (size_t j = 0; j < list_size(wrapper->bodies); j++) {
            body_t *body = list_get(wrapper->bodies, j);
            if (body_is_removed(body)) {
                list_remove(scene->forces, i);
                force_creator_wrapper_free(wrapper);
                i--;
                break;
            }
        }
    }
    // body tick and body removal
    for (size_t i = 0; i < list_size(scene->bodies); i++) {
        body_t *body = list_get(scene->bodies, i);
        if (body_is_removed(body)) {
            body_t *removed = list_remove(scene->bodies, i);
            body_free(removed);
            i--;
        } else {
            body_tick(body, dt);
        }
    }
    // force application (post-tick)
    for (size_t i = 0; i < list_size(scene->forces); i++) {
        force_creator_wrapper_t *wrapper = list_get(scene->forces, i);
        if (wrapper->is_post_tick) {
            wrapper->forcer(wrapper->aux);
        }
    }
}

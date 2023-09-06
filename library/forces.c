#include "forces.h"
#include "collision.h"
#include "test_util.h"
#include "utils.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>

const double NEWTONIAN_GRAVITY_MIN_DISTANCE = 5;
// Extra offset added in instant collision resolution. If negative,
// the bodies will stay slightly collided.
const double INSTANT_COLLISION_RESOLUTION_EPSILON = -0.01;

typedef struct one_body_force_params {
    double force_constant;
    body_t *body;
} one_body_force_params_t;

typedef struct two_body_force_params {
    double force_constant;
    body_t *body1;
    body_t *body2;
} two_body_force_params_t;

typedef struct physical_constraint_force_params {
    vector_t displacement;
    body_t *body1;
    body_t *body2;
} physical_constraint_force_params_t;

/**
 * body1 - the fist body involved in the collision
 * body2 - the second body involved in the collision
 * collision_handler - the function to be called when the bodies collide
 * collided_in_last_frame - a field that is updated to remember if a collision
 * has occurred in the last frame
 * is_contact_collision - if true, the handler is applied even if the bodies
 * collided in the last frame.
 * is_full_collision - if true, the handler is only applied if the collision is
 * full - that is, the bodies fully overlap
 * aux - parameters passed to collision_handler
 * freer - a function to free aux
 */
typedef struct collision_force_params {
    body_t *body1;
    body_t *body2;
    collision_handler_t collision_handler;
    bool collided_in_last_frame;
    bool is_contact_collision;
    bool is_full_collision;
    void *aux;
    free_func_t freer;
} collision_force_params_t;

typedef struct special_interaction_force_params {
    body_t *body1;
    body_t *body2;
    special_interaction_handler_t handler;
    void *aux;
    free_func_t freer;
} special_interaction_force_params_t;

one_body_force_params_t *one_body_force_params_init(double force_constant,
                                                    body_t *body) {
    one_body_force_params_t *result = malloc(sizeof(one_body_force_params_t));
    result->force_constant = force_constant;
    result->body = body;
    return result;
}

two_body_force_params_t *two_body_force_params_init(double force_constant,
                                                    body_t *body1,
                                                    body_t *body2) {

    two_body_force_params_t *result = malloc(sizeof(two_body_force_params_t));
    result->force_constant = force_constant;
    result->body1 = body1;
    result->body2 = body2;
    return result;
}

physical_constraint_force_params_t *
physical_constraint_force_params_init(vector_t displacement, body_t *body1,
                                      body_t *body2) {
    physical_constraint_force_params_t *result =
        malloc(sizeof(physical_constraint_force_params_t));
    result->displacement = displacement;
    result->body1 = body1;
    result->body2 = body2;
    return result;
}

collision_force_params_t *
collision_force_params_init(body_t *body1, body_t *body2,
                            collision_handler_t collision_handler,
                            bool is_contact_collision, bool is_full_collision,
                            void *aux, free_func_t freer) {
    collision_force_params_t *result = malloc(sizeof(collision_force_params_t));
    result->body1 = body1;
    result->body2 = body2;
    result->collision_handler = collision_handler;
    result->is_contact_collision = is_contact_collision;
    result->is_full_collision = is_full_collision;
    result->collided_in_last_frame = false;
    result->aux = aux;
    result->freer = freer;
    return result;
}

special_interaction_force_params_t *
special_interaction_force_params_init(body_t *body1, body_t *body2,
                                      special_interaction_handler_t handler,
                                      void *aux, free_func_t freer) {
    special_interaction_force_params_t *result =
        malloc(sizeof(special_interaction_force_params_t));
    result->body1 = body1;
    result->body2 = body2;
    result->handler = handler;
    result->aux = aux;
    result->freer = freer;
    return result;
}

void collision_force_params_free(collision_force_params_t *params) {
    if (params->freer) {
        params->freer(params->aux);
    }
    free(params);
}

void special_iteraction_force_params_free(
    special_interaction_force_params_t *params) {
    if (params->freer) {
        params->freer(params->aux);
    }
    free(params);
}

void newtonian_gravity_force_creator(two_body_force_params_t *gravity_params) {
    vector_t pos1 = body_get_centroid(gravity_params->body1);
    vector_t pos2 = body_get_centroid(gravity_params->body2);

    double dist = vec_distance(pos1, pos2);
    if (dist < NEWTONIAN_GRAVITY_MIN_DISTANCE) {
        return;
    }
    double force_magnitude =
        (gravity_params->force_constant * body_get_mass(gravity_params->body1) *
         body_get_mass(gravity_params->body2)) /
        (dist * dist);
    vector_t force1 =
        vec_multiply(force_magnitude, vec_direction(vec_subtract(pos2, pos1)));
    vector_t force2 = vec_negate(force1);
    body_add_force(gravity_params->body1, force1);
    body_add_force(gravity_params->body2, force2);
}

void create_newtonian_gravity(scene_t *scene, double G, body_t *body1,
                              body_t *body2) {
    list_t *bodies = list_init(2, NULL);
    list_add(bodies, body1);
    list_add(bodies, body2);
    scene_add_bodies_force_creator(
        scene, (force_creator_t)newtonian_gravity_force_creator,
        two_body_force_params_init(G, body1, body2), bodies, free);
}

void global_gravity_force_creator(one_body_force_params_t *gravity_params) {
    vector_t force = {.x = 0,
                      .y = -body_get_mass(gravity_params->body) *
                           gravity_params->force_constant};
    body_add_force(gravity_params->body, force);
}

void create_global_gravity(scene_t *scene, double g, body_t *body) {
    list_t *bodies = list_init(1, NULL);
    list_add(bodies, body);
    scene_add_bodies_force_creator(
        scene, (force_creator_t)global_gravity_force_creator,
        one_body_force_params_init(g, body), bodies, free);
}

void spring_force_creator(two_body_force_params_t *spring_params) {
    vector_t pos1 = body_get_centroid(spring_params->body1);
    vector_t pos2 = body_get_centroid(spring_params->body2);

    double dist = vec_distance(pos1, pos2);
    double force_magnitude = (spring_params->force_constant * dist);
    vector_t force1 =
        vec_multiply(force_magnitude, vec_direction(vec_subtract(pos2, pos1)));
    vector_t force2 = vec_negate(force1);
    body_add_force(spring_params->body1, force1);
    body_add_force(spring_params->body2, force2);
}

void create_spring(scene_t *scene, double k, body_t *body1, body_t *body2) {
    list_t *bodies = list_init(2, NULL);
    list_add(bodies, body1);
    list_add(bodies, body2);
    scene_add_bodies_force_creator(scene, (force_creator_t)spring_force_creator,
                                   two_body_force_params_init(k, body1, body2),
                                   bodies, free);
}

void drag_force_creator(one_body_force_params_t *drag_params) {
    vector_t force = vec_negate(vec_multiply(
        drag_params->force_constant, body_get_velocity(drag_params->body)));
    body_add_force(drag_params->body, force);
}

void create_drag(scene_t *scene, double gamma, body_t *body) {
    list_t *bodies = list_init(2, NULL);
    list_add(bodies, body);
    scene_add_bodies_force_creator(scene, (force_creator_t)drag_force_creator,
                                   one_body_force_params_init(gamma, body),
                                   bodies, free);
}

void friction_collision_handler(body_t *body1, body_t *body2,
                                vector_t collision_axis, double *mu) {
    vector_t parallel_axis = vec_rotate(collision_axis, PI / 2);
    vector_t relative_velocity =
        vec_subtract(body_get_velocity(body1), body_get_velocity(body2));
    vector_t parallel_component =
        vec_multiply(vec_dot(parallel_axis, relative_velocity), parallel_axis);
    vector_t friction_force1 =
        vec_multiply(-*mu, vec_direction(parallel_component));
    vector_t friction_force2 = vec_negate(friction_force1);
    body_add_force(body1, friction_force1);
    body_add_force(body2, friction_force2);
}

void create_friction(scene_t *scene, double mu, body_t *body1, body_t *body2) {
    double *mu_aux = malloc(sizeof(double));
    *mu_aux = mu;
    create_contact_collision(scene, body1, body2,
                             (collision_handler_t)friction_collision_handler,
                             mu_aux, free);
}

void physical_rigid_constraint_force_creator(
    physical_constraint_force_params_t *params) {
    vector_t real_displacement = vec_subtract(body_get_centroid(params->body1),
                                              body_get_centroid(params->body2));
    double m1 = body_get_mass(params->body1);
    double m2 = body_get_mass(params->body2);
    if (m1 == INFINITY && m2 == INFINITY) {
        return;
    }
    double reduced_mass = (m1 == INFINITY)   ? m2
                          : (m2 == INFINITY) ? m1
                                             : ((m1 * m2) / (m1 + m2));
    vector_t translation1 =
        vec_multiply(reduced_mass / m1,
                     vec_subtract(params->displacement, real_displacement));
    vector_t translation2 =
        vec_multiply(reduced_mass / m2,
                     vec_subtract(real_displacement, params->displacement));
    body_translate(params->body1, translation1);
    body_translate(params->body2, translation2);
}

void create_physical_rigid_constraint(scene_t *scene, body_t *body1,
                                      body_t *body2) {
    list_t *bodies = list_init(2, NULL);
    list_add(bodies, body1);
    list_add(bodies, body2);
    vector_t displacement =
        vec_subtract(body_get_centroid(body1), body_get_centroid(body2));
    scene_add_bodies_generic_force_creator(
        scene, (force_creator_t)physical_rigid_constraint_force_creator, true,
        physical_constraint_force_params_init(displacement, body1, body2),
        bodies, free);
}

void generic_collision_force_creator(collision_force_params_t *params) {
    collision_info_t info = detect_body_collision(params->body1, params->body2);
    if (info.collided) {
        // If contact collision, allow collision in last frame to activate.
        // If full collision, only activate on full collisions.
        if ((!params->collided_in_last_frame || params->is_contact_collision) &&
            (!params->is_full_collision || info.collided == FULL_COLLISION)) {
            params->collision_handler(params->body1, params->body2, info.axis,
                                      params->aux);
        }
        // For full, collisions, only count as collided if this is actually
        // a full collision
        if (!params->is_full_collision || info.collided == FULL_COLLISION) {
            params->collided_in_last_frame = true;
        }
    } else {
        params->collided_in_last_frame = false;
    }
}

void create_generic_collision(scene_t *scene, body_t *body1, body_t *body2,
                              collision_handler_t handler, bool is_post_tick,
                              bool is_contact_collision, bool is_full_collision,
                              void *aux, free_func_t freer) {
    list_t *bodies = list_init(2, NULL);
    list_add(bodies, body1);
    list_add(bodies, body2);

    collision_force_params_t *collision_force_params =
        collision_force_params_init(body1, body2, handler, is_contact_collision,
                                    is_full_collision, aux, freer);

    scene_add_bodies_generic_force_creator(
        scene, (force_creator_t)generic_collision_force_creator, is_post_tick,
        collision_force_params, bodies,
        (free_func_t)collision_force_params_free);
}

void create_collision(scene_t *scene, body_t *body1, body_t *body2,
                      collision_handler_t handler, void *aux,
                      free_func_t freer) {
    create_generic_collision(scene, body1, body2, handler, false, false, false,
                             aux, freer);
}

void create_contact_collision(scene_t *scene, body_t *body1, body_t *body2,
                              collision_handler_t handler, void *aux,
                              free_func_t freer) {
    create_generic_collision(scene, body1, body2, handler, false, true, false,
                             aux, freer);
}

void destructive_collision_handler(body_t *body1, body_t *body2, vector_t axis,
                                   void *aux) {
    body_remove(body1);
    body_remove(body2);
}

void create_destructive_collision(scene_t *scene, body_t *body1,
                                  body_t *body2) {
    create_collision(scene, body1, body2, destructive_collision_handler, NULL,
                     NULL);
}

void one_body_full_destructive_collision_force_creator(
    two_body_force_params_t *collision_params) {
    if (detect_body_collision(collision_params->body1, collision_params->body2)
            .collided == FULL_COLLISION) {
        body_remove(collision_params->body1);
    }
}

void one_body_destructive_collision_handler(body_t *body1, body_t *body2,
                                            vector_t axis, void *aux) {
    body_remove(body1);
}

void create_one_body_full_destructive_collision(scene_t *scene,
                                                body_t *body_to_be_destroyed,
                                                body_t *other_body) {
    create_generic_collision(scene, body_to_be_destroyed, other_body,
                             one_body_destructive_collision_handler, false,
                             true, true, NULL, NULL);
}

vector_t get_physics_collision_impulse(body_t *body1, body_t *body2,
                                       vector_t axis, double elasticity) {
    double m1 = body_get_mass(body1);
    double m2 = body_get_mass(body2);
    if (m1 == INFINITY && m2 == INFINITY) {
        return VEC_ZERO;
    }
    double reduced_mass = (m1 == INFINITY)   ? m2
                          : (m2 == INFINITY) ? m1
                                             : ((m1 * m2) / (m1 + m2));
    double u1 = vec_dot(body_get_velocity(body1), axis);
    double u2 = vec_dot(body_get_velocity(body2), axis);
    double impulse_proj = reduced_mass * (1 + elasticity) * (u2 - u1);
    vector_t impulse = vec_multiply(impulse_proj, axis);
    return impulse;
}

void physics_collision_handler(body_t *body1, body_t *body2, vector_t axis,
                               double *elasticity) {
    vector_t impulse1 =
        get_physics_collision_impulse(body1, body2, axis, *elasticity);
    vector_t impulse2 = vec_negate(impulse1);
    body_add_impulse(body1, impulse1);
    body_add_impulse(body2, impulse2);
}

void create_physics_collision(scene_t *scene, double elasticity, body_t *body1,
                              body_t *body2) {
    double *elasticity_aux = malloc(sizeof(double));
    *elasticity_aux = elasticity;
    create_collision(scene, body1, body2,
                     (collision_handler_t)physics_collision_handler,
                     elasticity_aux, free);
}

void create_physics_contact_collision(scene_t *scene, double elasticity,
                                      body_t *body1, body_t *body2) {
    double *elasticity_aux = malloc(sizeof(double));
    *elasticity_aux = elasticity;
    create_contact_collision(scene, body1, body2,
                             (collision_handler_t)physics_collision_handler,
                             elasticity_aux, free);
}

void instant_resolution_collision_handler(body_t *body1, body_t *body2,
                                          vector_t axis, void *aux) {
    // If it's a full collision, we can't do anything - give up
    if (detect_body_collision(body1, body2).collided == FULL_COLLISION) {
        return;
    }
    double m1 = body_get_mass(body1);
    double m2 = body_get_mass(body2);
    if (m1 == INFINITY && m2 == INFINITY) {
        return;
    }
    double reduced_mass = (m1 == INFINITY)   ? m2
                          : (m2 == INFINITY) ? m1
                                             : ((m1 * m2) / (m1 + m2));
    double overlap = detect_body_collision(body1, body2).overlap;
    double translation_body1 =
        -reduced_mass / m1 * (overlap + INSTANT_COLLISION_RESOLUTION_EPSILON);
    double translation_body2 =
        reduced_mass / m2 * (overlap + INSTANT_COLLISION_RESOLUTION_EPSILON);
    vector_t trans_vector_body1 = vec_multiply(translation_body1, axis);
    vector_t trans_vector_body2 = vec_multiply(translation_body2, axis);
    body_translate(body1, trans_vector_body1);
    body_translate(body2, trans_vector_body2);
    // Simulate a physics collision of elasticity 0 instantaneously
    vector_t impulse1 = get_physics_collision_impulse(body1, body2, axis, 0);
    vector_t impulse2 = vec_negate(impulse1);
    body_set_velocity(body1, vec_add(body_get_velocity(body1), vec_multiply(1 / m1, impulse1)));
    body_set_velocity(body2, vec_add(body_get_velocity(body2), vec_multiply(1 / m2, impulse2)));
}

void create_instant_resolution_collision(scene_t *scene, body_t *body1,
                                         body_t *body2) {
    if (body_get_mass(body1) != INFINITY || body_get_mass(body2) != INFINITY) {
        create_generic_collision(
            scene, body1, body2,
            (collision_handler_t)instant_resolution_collision_handler, true,
            true, false, NULL, free);
    }
}

void special_interaction_force_creator(
    special_interaction_force_params_t *params) {
    params->handler(params->body1, params->body2, params->aux);
}

void create_special_interaction(scene_t *scene, body_t *body1, body_t *body2,
                                special_interaction_handler_t handler,
                                bool is_post_tick, void *aux,
                                free_func_t freer) {
    list_t *bodies = list_init(2, NULL);
    list_add(bodies, body1);
    list_add(bodies, body2);

    special_interaction_force_params_t *interaction_force_params =
        special_interaction_force_params_init(body1, body2, handler, aux,
                                              freer);

    scene_add_bodies_generic_force_creator(
        scene, (force_creator_t)special_interaction_force_creator, is_post_tick,
        interaction_force_params, bodies,
        (free_func_t)special_iteraction_force_params_free);
}

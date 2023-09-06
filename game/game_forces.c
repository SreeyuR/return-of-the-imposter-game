#include "game_forces.h"
#include "body.h"
#include "forces.h"
#include "game_body_info.h"
#include "game_constants.h"
#include "game_gui.h"
#include "game_load_level.h"
#include "game_timers.h"
#include "polygon.h"
#include "sdl_wrapper.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>

#define TOUCHING_GROUND_VELOCITY_THRESHOLD 0.001

void level_winning_collision_handler(body_t *player, body_t *vent,
                                     vector_t collision_axis, state_t *state) {
    sdl_play_sound_effect(WON_LEVEL_SOUND_FILEPATH, false);
    printf("The imposter wins! Very sus.\n");
    if (state->curr_level == NUM_LEVELS - 1) {
        printf("You have completed all the levels!\n");
        load_victory_screen(state);
    } else {
        state->curr_level++;
        load_level(state);
    }
}

void player_ground_collision_handler(body_t *player, body_t *ground,
                                     vector_t collision_axis, state_t *state) {
    player_info_t *player_info = body_get_info(player);
    // useful for when the player is, for ex, on a vertically moving platform
    vector_t relative_velocity =
        vec_subtract(body_get_velocity(player), body_get_velocity(ground));
    if (collision_axis.y < 0 &&
        fabs(collision_axis.y) > fabs(collision_axis.x) &&
        relative_velocity.y <= TOUCHING_GROUND_VELOCITY_THRESHOLD) {
        player_info->player_touching_ground = true;
    }
}

void player_trampoline_collision_handler(body_t *player, body_t *trampoline,
                                         vector_t *collision_axis,
                                         state_t *state) {
    assert(get_role(player) == PLAYER);
    assert(get_role(trampoline) == TRAMPOLINE);
    trampoline_info_t *trampoline_info = body_get_info(trampoline);
    body_add_impulse(player,
                     (vector_t){.x = 0, .y = trampoline_info->bounciness});
}

void tongue_tip_collision_handler(body_t *tongue_tip, body_t *to_attach,
                                  vector_t collision_axis, state_t *state) {
    assert(get_role(tongue_tip) == TONGUE_TIP &&
           get_role(to_attach) & (WALL | DOOR));
    // Note that the tongue status could be DEPLOYED, ATTACHED (if it already
    // collided with another body in the same frame), or even CHARGING (if this
    // is the frame right after it was removed)
    body_t *player = state->player;
    assert(player);
    player_info_t *player_info = body_get_info(player);
    // Make the tongue stick to the other body
    if (player_info->tongue_status == DEPLOYED) {
        create_spring(state->scene, TONGUE_ATTACHED_SPRING_CONSTANT, player,
                      tongue_tip);
        body_set_mass(tongue_tip, INFINITY);
        body_set_velocity(tongue_tip, VEC_ZERO);
        player_info->tongue_status = ATTACHED;
    }
}

void damaged_body_damager_collision_handler(body_t *damaged_body,
                                            body_t *damager,
                                            vector_t collision_axis,
                                            state_t *state) {
    assert(get_role(damaged_body) & (PLAYER | CREWMATE));
    assert(get_role(damager) & (TONGUE_TIP | DAMAGING_OBSTACLE | BULLET));
    body_health_info_t *health_info = get_health_info(damaged_body);
    damaging_body_info_t *damage_info = get_damaging_body_info(damager);
    if (health_info->invincibility_time_left <= 0 && damage_info->damage > 0) {
        assert(health_info->health > 0);
        health_info->health -= damage_info->damage;
        if (get_role(damaged_body) == PLAYER) {
            sdl_play_sound_effect(OOF_SOUND_FILEPATH, false);
        } else if (get_role(damaged_body) == CREWMATE) {
            sdl_play_sound_effect(OW_SOUND_FILEPATH, false);
        }
        if (health_info->health <= 0) {
            if (get_role(damaged_body) == PLAYER) {
                damaging_obstacle_info_t *damaging_obstacle_info =
                    (damaging_obstacle_info_t *)body_get_info(damager);
                if (damaging_obstacle_info->remove_upon_collision) {
                    body_remove(damager);
                }
                sdl_play_sound_effect(LEVEL_FAILED_SOUND_FILEPATH, true);
                printf("%s\n", damaging_obstacle_info->game_over_message);
                add_timer(state, GAME_OVER_TIME_DELAY, load_level);
                state->game_status = DEATH;
            } else if (get_role(damaged_body) == CREWMATE) {
                sdl_play_sound_effect(CREWMATE_DEATH_SOUND_FILEPATH, false);
                body_remove(damaged_body);
            }
        } else {
            health_info->invincibility_time_left =
                health_info->total_invincibility_time;
        }
    }
}

void bullet_collision_handler(body_t *bullet, body_t *other_body,
                              vector_t collision_axis, state_t *state) {
    assert(get_role(bullet) == BULLET);
    body_remove(bullet);
}

void player_key_collision_handler(body_t *player, body_t *key,
                                  vector_t collision_axis, state_t *state) {
    assert(get_role(player) == PLAYER);
    assert(get_role(key) == KEY);
    player_info_t *player_info = body_get_info(player);
    key_and_door_info_t *key_info = body_get_info(key);
    size_t *id = malloc(sizeof(size_t));
    *id = key_info->id;
    sdl_play_sound_effect(KEY_COLLECTED_SOUND_FILEPATH, false);
    list_add(player_info->key_ids_collected, id);
    body_remove(key);
}

void player_door_collision_handler(body_t *player, body_t *door,
                                   vector_t collision_axis, state_t *state) {
    assert(get_role(player) == PLAYER);
    assert(get_role(door) == DOOR);
    player_info_t *player_info = body_get_info(player);
    key_and_door_info_t *door_info = body_get_info(door);
    size_t door_id = door_info->id;
    for (size_t i = 0; i < list_size(player_info->key_ids_collected); i++) {
        size_t curr_id = *(size_t *)list_get(player_info->key_ids_collected, i);
        if (curr_id == door_id) {
            // we can only open the door when we have the right key
            sdl_play_sound_effect(OPEN_DOOR_SOUND_FILEPATH, false);
            free(list_remove(player_info->key_ids_collected, i));
            body_remove(door);
            break;
        }
    }
}

void create_asymmetric_interaction_helper(
    scene_t *scene, body_t *body1, body_t *body2, body_role_t expected_role_1,
    body_role_t expected_role_2, special_interaction_handler_t handler,
    bool is_post_tick, void *aux, free_func_t freer) {
    body_role_t role1 = get_role(body1);
    body_role_t role2 = get_role(body2);
    assert((role1 == expected_role_1 && role2 == expected_role_2) ||
           (role1 == expected_role_2 && role2 == expected_role_1));
    if (role1 == expected_role_1) {
        create_special_interaction(scene, body1, body2, handler, is_post_tick,
                                   aux, freer);
    } else {
        create_special_interaction(scene, body2, body1, handler, is_post_tick,
                                   aux, freer);
    }
}

void create_asymmetric_collision(scene_t *scene, body_t *body1, body_t *body2,
                                 body_role_t expected_role_1,
                                 body_role_t expected_role_2,
                                 collision_handler_t handler, bool is_post_tick,
                                 bool is_contact_collision,
                                 bool is_full_collision, bool resolve_collision,
                                 void *aux, free_func_t freer) {
    body_role_t role1 = get_role(body1);
    body_role_t role2 = get_role(body2);
    if ((role1 & expected_role_1) && (role2 & expected_role_2)) {
        if (handler) {
            create_generic_collision(scene, body1, body2, handler, is_post_tick,
                                     is_contact_collision, is_full_collision,
                                     aux, freer);
        }
    } else if ((role1 & expected_role_2) && (role2 & expected_role_1)) {
        if (handler) {
            create_generic_collision(scene, body2, body1, handler, is_post_tick,
                                     is_contact_collision, is_full_collision,
                                     aux, freer);
        }
    } else {
        return;
    }
    if (resolve_collision) {
        create_instant_resolution_collision(scene, body1, body2);
    }
}

// // prevent player from experiencing an impulse from the sides of a trampoline.
// void add_trampoline_side_walls(state_t *state, body_t *trampoline) {
//     assert(get_role(trampoline) == TRAMPOLINE);
//     double min_x = body_get_bounding_box(trampoline).min_x;
//     double max_x = body_get_bounding_box(trampoline).max_x;
//     double min_y = body_get_bounding_box(trampoline).min_y;
//     double max_y = body_get_bounding_box(trampoline).max_y;
//     double wall_width = (max_x - min_x) / TRAMPOLINE_SIDE_WALL_THICKNESS_RATIO;
//     double wall_height = max_y - min_y;
//     // left wall
//     vector_t left_wall_pos = {.x = min_x, .y = body_get_centroid(trampoline).y};
//     list_t *left_wall_shape = initialize_rectangle_anchored(
//         (anchor_option_t){.x_anchor = ANCHOR_MAX, .y_anchor = ANCHOR_CENTER},
//         left_wall_pos, wall_width, wall_height);
//     body_t *left_wall = body_init(left_wall_shape, TRAMPOLINE_SIDE_WALL_MASS,
//                                   COLOR_TRANSPARENT);
//     // right wall
//     vector_t right_wall_pos = {.x = max_x,
//                                .y = body_get_centroid(trampoline).y};
//     list_t *right_wall_shape = initialize_rectangle_anchored(
//         (anchor_option_t){.x_anchor = ANCHOR_MIN, .y_anchor = ANCHOR_CENTER},
//         right_wall_pos, wall_width, wall_height);
//     body_t *right_wall = body_init(right_wall_shape, TRAMPOLINE_SIDE_WALL_MASS,
//                                    COLOR_TRANSPARENT);
//     // add walls
//     scene_add_body(state->scene, left_wall);
//     scene_add_body(state->scene, right_wall);
// }

void add_body_with_forces(state_t *state, body_t *new_body) {
    body_role_t new_body_role = get_role(new_body);
    if (new_body_role & (BULLET | TONGUE | TONGUE_TIP)) {
        create_global_gravity(state->scene, BULLET_GRAVITY_ACCELERATION,
                              new_body);
    } else {
        create_global_gravity(state->scene, GRAVITY_ACCELERATION, new_body);
    }

    if (new_body_role == PLAYER) {
        create_drag(state->scene, PLAYER_DRAG_CONSTANT, new_body);
    } else if (new_body_role & (TONGUE | TONGUE_TIP)) {
        create_drag(state->scene, TONGUE_DRAG_CONSTANT, new_body);
    }

    for (size_t i = 0; i < scene_bodies(state->scene); i++) {
        body_t *old_body = scene_get_body(state->scene, i);
        body_role_t old_body_role = get_role(old_body);
        // Friction forces
        if ((old_body_role | new_body_role) == (PLAYER | WALL)) {
            create_friction(state->scene, FRICTION_COEFFICIENT, old_body,
                            new_body);
        }
        if ((old_body_role | new_body_role) == (PLAYER | TRAMPOLINE)) {
            trampoline_info_t *trampoline_info = old_body_role == TRAMPOLINE
                ? body_get_info(old_body) : body_get_info(new_body);
            double elasticity = trampoline_info->bounciness;
            create_physics_collision(state->scene, elasticity, old_body,
                                     new_body);
        }
        // Level winning for vent
        create_asymmetric_collision(
            state->scene, new_body, old_body, PLAYER, VENT,
            (collision_handler_t)level_winning_collision_handler, false, true,
            false, false, state, NULL);
        // Instant resolution collision with solids
        // Only do when one of the solids is non-stationary
        create_asymmetric_collision(
            state->scene, new_body, old_body, SOLID & ~BULLET & ~CREWMATE,
            WALL | DOOR | VENT | DAMAGING_OBSTACLE | CREWMATE, NULL, false,
            true, false, true, state, NULL);
        // Collisions marking player standing on the ground
        create_asymmetric_collision(
            state->scene, new_body, old_body, PLAYER,
            WALL | DOOR | DAMAGING_OBSTACLE | TRAMPOLINE,
            (collision_handler_t)player_ground_collision_handler, false, true,
            false, false, state, NULL);
        // Tongue tip attachment to pull player
        create_asymmetric_collision(
            state->scene, new_body, old_body, TONGUE_TIP, WALL | DOOR,
            (collision_handler_t)tongue_tip_collision_handler, false, false,
            false, false, state, NULL);
        // Damaging obstacles and bullets damage player
        create_asymmetric_collision(
            state->scene, new_body, old_body, PLAYER,
            DAMAGING_OBSTACLE | BULLET,
            (collision_handler_t)damaged_body_damager_collision_handler, false,
            true, false, true, state, NULL);
        // Tongue tip damages crewmates
        create_asymmetric_collision(
            state->scene, new_body, old_body, CREWMATE, TONGUE_TIP,
            (collision_handler_t)damaged_body_damager_collision_handler, false,
            true, false, true, state, NULL);
        // Bullets disappear on contact with solids
        create_asymmetric_collision(
            state->scene, new_body, old_body, BULLET, SOLID & ~CREWMATE,
            (collision_handler_t)bullet_collision_handler, false, true, false,
            false, state, NULL);
        // Player collects keys
        create_asymmetric_collision(
            state->scene, new_body, old_body, PLAYER, KEY,
            (collision_handler_t)player_key_collision_handler, false, true,
            false, false, state, NULL);
        // Player opens doors (with keys)
        create_asymmetric_collision(
            state->scene, new_body, old_body, PLAYER, DOOR,
            (collision_handler_t)player_door_collision_handler, false, true,
            false, false, state, NULL);
    }
    scene_add_body(state->scene, new_body);
}

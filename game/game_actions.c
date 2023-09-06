#include "game_actions.h"
#include "forces.h"
#include "game.h"
#include "game_body_info.h"
#include "game_constants.h"
#include "game_forces.h"
#include "game_gui.h"
#include "game_load_level.h"
#include "polygon.h"
#include "sdl_wrapper.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#if defined __EMSCRIPTEN__ && defined DEBUG_KEYPRESSES
#include <emscripten.h>
#endif

#define CREWMATE_DIRECTION_CHANGE_THRESHOLD 0.001

body_t *create_bullet(state_t *state, body_t *crewmate) {
    assert(get_role(crewmate) == CREWMATE);
    crewmate_info_t *crewmate_info = (crewmate_info_t *)body_get_info(crewmate);
    vector_t bullet_init_loc = {
        .x = body_get_centroid(crewmate).x + BULLET_INIT_HORIZONTAL_OFFSET,
        .y = body_get_centroid(crewmate).y + BULLET_INIT_VERTICAL_OFFSET};
    list_t *bullet_shape = initialize_rectangle_centered(
        bullet_init_loc, BULLET_WIDTH, BULLET_HEIGHT);
    damaging_obstacle_info_t *bullet_info = damaging_obstacle_info_init(
        BULLET, crewmate_info->damage_per_bullet, NULL, true,
        crewmate_info->game_over_message);
    body_t *bullet = body_init_with_info(bullet_shape, BULLET_MASS,
                                         BULLET_COLOR, bullet_info, free);
    add_body_with_forces(state, bullet);
    return bullet;
}

void fire_bullet(state_t *state, body_t *crewmate, body_t *bullet) {
    sdl_play_sound_effect(BULLET_SOUND_FILEPATH, false);
    assert(get_role(bullet) == BULLET);
    vector_t bullet_direction = vec_direction(vec_subtract(
        body_get_centroid(state->player), body_get_centroid(bullet)));
    vector_t bullet_velocity = vec_multiply(BULLET_SPEED, bullet_direction);
    body_set_velocity(bullet, bullet_velocity);
}

bool crewmate_line_of_sight_opaqueness(body_t *body) {
    // crewmate should not be able to see through a wall or damaging obstacle
    // it SHOULD be able to see through bullets though. That's why we don't list
    // `BULLET` here.
    return get_role(body) & (WALL | DAMAGING_OBSTACLE | CREWMATE | DOOR);
}

void crewmate_attack_player(state_t *state, body_t *crewmate, double dt) {
    crewmate_info_t *crewmate_info = (crewmate_info_t *)body_get_info(crewmate);
    bool crewmate_facing_player =
        (crewmate_info->facing_left &&
         body_get_centroid(state->player).x < body_get_centroid(crewmate).x) ||
        (!crewmate_info->facing_left &&
         body_get_centroid(state->player).x > body_get_centroid(crewmate).x);
    if (crewmate_facing_player &&
        scene_detect_line_of_sight(state->scene, crewmate, state->player,
                                   crewmate_line_of_sight_opaqueness)) {
        if (crewmate_info->reloading_timer <= 0) {
            fire_bullet(state, crewmate, create_bullet(state, crewmate));
            crewmate_info->reloading_timer = crewmate_info->reload_time;
        } else {
            crewmate_info->reloading_timer -= dt;
        }
    }
}

void update_body_trajectory(body_t *body) {
    assert(get_role(body) & (CREWMATE | DAMAGING_OBSTACLE));
    trajectory_info_t *trajectory_info = get_trajectory(body);
    if (!trajectory_info) {
        return; // no trajectory associated with body
    }
    list_t *trajectory_shape = trajectory_info->trajectory_shape;
    vector_t curr_point = get_curr_trajectory_point(body);
    vector_t next_point = get_next_trajectory_point(body);
    vector_t body_centroid = body_get_centroid(body);
    // move to the next point when body passes target point of trajectory.
    if (vec_dot(vec_subtract(next_point, curr_point),
                vec_subtract(next_point, body_centroid)) <= 0) {
        trajectory_info->curr_point_index++;
        trajectory_info->curr_point_index %= list_size(trajectory_shape);
    }
    vector_t direction = vec_direction(vec_subtract(next_point, body_centroid));
    vector_t new_velocty = vec_multiply(trajectory_info->speed, direction);
    body_set_velocity(body, new_velocty);
}

void pseudo_rotation_mech(body_t *prev_tongue, body_t *curr_tongue,
                          state_t *state) {
    assert(get_role(prev_tongue) & (TONGUE_TIP | TONGUE));
    assert(get_role(curr_tongue) & (TONGUE_TIP | TONGUE));
    body_role_t role = DECORATION;
    body_info_t *tongue_info = body_info_init(role);
    body_t *temp_rectangle =
        body_init_with_info(initialize_rectangle_rotated(
                                body_get_centroid(prev_tongue),
                                body_get_centroid(curr_tongue), TONGUE_WIDTH),
                            TONGUE_PIECE_MASS, TONGUE_COLOR, tongue_info, free);
    scene_add_body(state->scene, temp_rectangle);
    body_remove(temp_rectangle);
}

void deploy_tongue(state_t *state, vector_t velocity) {
    body_t *player = state->player;
    assert(player);
    player_info_t *player_info = body_get_info(player);
    if (player_info->tongue_status != READY) {
        return;
    }
    sdl_play_sound_effect(TONGUE_SOUND_FILEPATH, false);
    player_info->tongue_status = DEPLOYED;
    player_info->tongue_timer = TONGUE_DEPLOYMENT_TIME;
    // TODO: in the future, this not necessarily be the centroid but
    // wherever the mouth of the player is
    vector_t spawn_position = body_get_centroid(player);
    assert(TONGUE_NUM_PIECES >= 2);
    body_t *prev = NULL;
    for (size_t i = 0; i < TONGUE_NUM_PIECES; i++) {
        body_role_t role = i == TONGUE_NUM_PIECES - 1 ? TONGUE_TIP : TONGUE;
        body_info_t *tongue_info =
            role == TONGUE_TIP ? (body_info_t *)damaging_body_info_init(
                                     TONGUE_TIP, player_info->tongue_damage)
                               : body_info_init(role);
        body_t *tongue_piece = body_init_with_info(
            initialize_rectangle_anchored(
                (anchor_option_t){.x_anchor = ANCHOR_MIN,
                                  .y_anchor = ANCHOR_CENTER},
                spawn_position, TONGUE_WIDTH, TONGUE_WIDTH),
            TONGUE_PIECE_MASS, TONGUE_COLOR, tongue_info, free);
        if (prev) {
            create_spring(state->scene, TONGUE_SPRING_CONSTANT, prev,
                          tongue_piece);
            create_special_interaction(
                state->scene, prev, tongue_piece,
                (special_interaction_handler_t)pseudo_rotation_mech, true,
                state, NULL);
        } else {
            create_spring(state->scene, TONGUE_SPRING_CONSTANT, tongue_piece,
                          player);
        }
        add_body_with_forces(state, tongue_piece);
        // Initialize velocities to go from 0 to velocity along the length of
        // the tongue
        body_set_velocity(
            tongue_piece,
            vec_multiply(((double)i / TONGUE_NUM_PIECES), velocity));
        prev = tongue_piece;
    }
}

void remove_tongue(state_t *state) {
    body_t *player = state->player;
    assert(player);
    player_info_t *player_info = body_get_info(player);
    assert(player_info->tongue_status == DEPLOYED ||
           player_info->tongue_status == ATTACHED);
    for (size_t i = 0; i < scene_bodies(state->scene); i++) {
        body_t *body = scene_get_body(state->scene, i);
        if (get_role(body) & (TONGUE | TONGUE_TIP)) {
            body_remove(body);
        }
    }
}

void body_health_invincibility_effect(state_t *state, body_t *body, double dt) {
    body_health_info_t *health_info = get_health_info(body);
    if (health_info->invincibility_time_left > 0) {
        health_info->invincibility_time_left -= dt;
        if (fmod(health_info->invincibility_time_left,
                 INVINCIBILITY_BLINKING_TIME) <
            (INVINCIBILITY_BLINKING_TIME / 2.0)) {
            body_set_visibility(body, false);
        } else {
            body_set_visibility(body, true);
        }
    } else {
        body_set_visibility(body, true);
    }
}

void handle_tongue_timer(state_t *state, double dt) {
    body_t *player = state->player;
    assert(player);
    player_info_t *player_info = body_get_info(player);
    if (player_info->tongue_status == READY) {
        return;
    }
    player_info->tongue_timer -= dt;
    if (player_info->tongue_timer <= 0) {
        if (player_info->tongue_status == CHARGING) {
            player_info->tongue_status = READY;
            player_info->tongue_timer = 0;
        } else if (player_info->tongue_status == DEPLOYED ||
                   player_info->tongue_status == ATTACHED) {
            remove_tongue(state);
            player_info->tongue_status = CHARGING;
            player_info->tongue_timer = TONGUE_CHARGE_TIME;
        }
    }
}

body_t *get_player_paparazzi(state_t *state) {
    assert(state->player);
    for (size_t i = 0; i < scene_bodies(state->scene); i++) {
        body_t *body = scene_get_body(state->scene, i);
        if (get_role(body) == PLAYER_PAPARAZZI) {
            return body;
        }
    }
    return NULL;
}

vector_t get_camera_for_player_pos(state_t *state) {
    assert(state->player);
    vector_t paparazzi_pos = body_get_centroid(get_player_paparazzi(state));
    vector_t player_pos = body_get_centroid(state->player);
    vector_t camera_pos =
        vec_add(player_pos, vec_subtract(player_pos, paparazzi_pos));
    return camera_pos;
}

// make sure paparazzi is close enough to the player so that the player is never
// off the window.
void update_paparazzi(state_t *state) {
    body_t *paparazzi = get_player_paparazzi(state);
    assert(paparazzi);
    vector_t player_pos = body_get_centroid(state->player);
    vector_t paparazzi_pos = body_get_centroid(paparazzi);
    // if the paprazzi went too far away, bring it back.
    if (vec_distance(player_pos, paparazzi_pos) > PLAYER_PAPARAZZI_MAX_RADIUS) {
        paparazzi_pos =
            vec_add(player_pos, vec_multiply(PLAYER_PAPARAZZI_MAX_RADIUS,
                                             vec_direction(vec_subtract(
                                                 paparazzi_pos, player_pos))));
        body_set_centroid(paparazzi, paparazzi_pos);
    }
}

void update_player_texture_direction(state_t *state) {
    assert(state->player);
    player_info_t *player_info = body_get_info(state->player);
    body_set_texture_flip(state->player, player_info->facing_left, false);
}

void update_player(state_t *state, double dt) {
    assert(state->player);
    player_info_t *player_info = body_get_info(state->player);
    player_info->player_touching_ground = false;
    body_health_invincibility_effect(state, state->player, dt);
    handle_tongue_timer(state, dt);
    update_player_texture_direction(state);
}

void update_crewmate_texture_direction(body_t *crewmate) {
    assert(crewmate);
    crewmate_info_t *crewmate_info = body_get_info(crewmate);
    body_set_texture_flip(crewmate, crewmate_info->facing_left, false);
}

void update_crewmate_direction(body_t *crewmate) {
    crewmate_info_t *crewmate_info = body_get_info(crewmate);
    vector_t velocity = body_get_velocity(crewmate);
    if (fabs(velocity.x) > CREWMATE_DIRECTION_CHANGE_THRESHOLD) {
        crewmate_info->facing_left = body_get_velocity(crewmate).x < 0;
    }
}

void update_crewmate(state_t *state, body_t *crewmate, double dt) {
    assert(get_role(crewmate) == CREWMATE);
    update_crewmate_direction(crewmate);
    body_health_invincibility_effect(state, crewmate, dt);
    update_body_trajectory(crewmate);
    crewmate_attack_player(state, crewmate, dt);
    update_crewmate_texture_direction(crewmate);
}

void update_damaging_obstacle(body_t *body) {
    assert(get_role(body) == DAMAGING_OBSTACLE);
    update_body_trajectory(body);
    return;
}

// This is separate from game_key_handler because key messages for a key being
// held are not sent when some other key is pressed during that time.
void handle_held_keys(state_t *state) {
    body_t *player = state->player;
    assert(player);
    player_info_t *player_info = body_get_info(player);
    // Keys that trigger while held
    if (state->held_keys['w']) { // jump
        if (player_info->player_touching_ground) {
            sdl_play_sound_effect(JUMP_SOUND_FILEPATH, false);
            body_add_impulse(player,
                             (vector_t){.x = 0, .y = PLAYER_JUMP_IMPULSE});
        }
    }
    if (state->held_keys['a']) { // left
        body_add_force(player, (vector_t){.x = -PLAYER_MOVE_FORCE, .y = 0});
        player_info->facing_left = true;
    }
    if (state->held_keys['d']) { // right
        body_add_force(player, (vector_t){.x = PLAYER_MOVE_FORCE, .y = 0});
        player_info->facing_left = false;
    }
}

void game_key_handler(state_t *state, unsigned char key, key_event_type_t type,
                      double held_time) {
    body_t *player = state->player;
    assert(player);
    bool previously_held = state->held_keys[key];
    if (type == KEY_PRESSED) {
        state->held_keys[key] = true;
        if (state->game_status == DEATH) {
            // No key input during death period (but still keep track of key
            // presses)
            return;
        }
        // For newly pressed keys that should only trigger once:
        if (!previously_held) {
            switch (key) {
                case 'p':
                    load_pause_menu(state);
                    break;
                case 'r':
                    // Press R to restart current level
                    load_level(state);
                    break;
#ifdef DEBUG_KEYPRESSES
                case 'x':
                    // Press X to forcefully crash the game
                    emscripten_free(state);
                    emscripten_force_exit(0);
                    break;
                case 'c':
                    // Press C to cheat (go to the next level)
                    if (state->curr_level != NUM_LEVELS - 1) {
                        state->curr_level++;
                    }
                    load_level(state);
                    break;
#endif
            }
        }
    } else if (type == KEY_RELEASED) {
        state->held_keys[key] = false;
    }
}

void game_mouse_handler(state_t *state, mouse_event_type_t type,
                        vector_t mouse_scene_pos,
                        vector_t mouse_prev_scene_pos) {
    if (state->game_status == DEATH) {
        // No mouse input during death period
        return;
    }
    body_t *player = state->player;
    assert(player);
    if (type == MOUSE_PRESSED) {
        vector_t mouse_dir =
            vec_subtract(mouse_scene_pos, body_get_centroid(player));
        double unit_mouse_dir =
            vec_distance(mouse_scene_pos, body_get_centroid(player));
        vector_t unit_mouse_coords =
            vec_multiply(TONGUE_INITIAL_SPEED / unit_mouse_dir, mouse_dir);
        deploy_tongue(state, unit_mouse_coords);
    }
}

void perform_game_actions(state_t *state, double dt) {
    body_t *player = state->player;
    assert(player);
    handle_held_keys(state);
    update_player(state, dt);
    update_paparazzi(state);
    for (size_t i = 0; i < scene_bodies(state->scene); i++) {
        body_t *body = scene_get_body(state->scene, i);
        switch (get_role(body)) {
            case DAMAGING_OBSTACLE:
                update_damaging_obstacle(body);
                break;
            case CREWMATE:
                update_crewmate(state, body, dt);
                break;
            default:
                break;
        }
    }
}

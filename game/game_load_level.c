#include "game_load_level.h"
#include "forces.h"
#include "game_actions.h"
#include "game_body_info.h"
#include "game_constants.h"
#include "game_forces.h"
#include "polygon.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

const int LEVEL_FILE_LINE_LENGTH = 1000;
const int LEVEL_FILE_ARG_LENGTH = 500;
const int ANCHOR_STR_LENGTH = 3; // 2 characters + null terminator

const char *LEVEL_FILE_DIR = "resources/levels/";

const char *LEVELS[] = {"level0.lvl", "level1.lvl", "level2.lvl", "level3.lvl", "level4.lvl", "level6.lvl", "level7.lvl"};

const size_t NUM_LEVELS = sizeof(LEVELS) / sizeof(char *);

anchor_option_t parse_anchor_option(char *anchor_str) {
    char cx = anchor_str[0];
    char cy = anchor_str[1];
    return (anchor_option_t){.x_anchor = cx == '-'   ? ANCHOR_MIN
                                         : cx == '+' ? ANCHOR_MAX
                                                     : ANCHOR_CENTER,
                             .y_anchor = cy == '-'   ? ANCHOR_MIN
                                         : cy == '+' ? ANCHOR_MAX
                                                     : ANCHOR_CENTER};
}

/**
 * Reads a named argument from a line of arguments (from a .lvl file)
 *
 * @param args The arguments of the line from which to read
 * @param arg_name The name of the argument to search for
 * @param result A pointer to a string in which to store the result
 * @param default_value The value to store in result if the argument is not
 * specified
 * @returns true if the argument was specified, false otherwise
 */
bool get_named_argument(char *args, char *arg_name, char *result,
                        const char *default_value) {
    char copy[strlen(args) + 1];
    strcpy(copy, args);
    char *token = strtok(copy, " ");
    bool found = false;
    while (token) {
        char name[LEVEL_FILE_ARG_LENGTH];
        char value[LEVEL_FILE_ARG_LENGTH];
        if (sscanf(token, "%[^=]=%s", name, value) == 2) {
            if (!strcmp(name, arg_name)) {
                strcpy(result, value);
                found = true;
                break;
            }
        }
        token = strtok(NULL, " ");
    }
    if (!found) {
        strcpy(result, default_value);
    }
    return found;
}

bool get_named_argument_str(char *args, char *arg_name, char *result,
                            const char *default_value) {
    if (get_named_argument(args, arg_name, result, default_value)) {
        for (size_t i = 0; i < strlen(result); i++) {
            // replace underscores with spaces in result
            if (result[i] == '_') {
                result[i] = ' ';
            }
        }
        return true;
    } else {
        return false;
    }
}

bool get_named_argument_double(char *args, char *arg_name, double *result,
                               double default_value) {
    char value[LEVEL_FILE_ARG_LENGTH];
    if (get_named_argument(args, arg_name, value, "")) {
        assert(sscanf(value, "%lf", result) == 1);
        return true;
    } else {
        *result = default_value;
        return false;
    }
}

bool get_named_argument_size_t(char *args, char *arg_name, size_t *result,
                               size_t default_value) {
    char value[LEVEL_FILE_ARG_LENGTH];
    if (get_named_argument(args, arg_name, value, "")) {
        assert(sscanf(value, "%zu", result) == 1);
        return true;
    } else {
        *result = default_value;
        return false;
    }
}

bool get_named_argument_color(char *args, char *arg_name, rgba_color_t *result,
                              rgba_color_t default_value) {
    char value[LEVEL_FILE_ARG_LENGTH];
    if (get_named_argument(args, arg_name, value, "")) {
        uint32_t hex;
        assert(sscanf(value, "%x", &hex) == 1);
        *result = hex_to_rgba(hex);
        return true;
    } else {
        *result = default_value;
        return false;
    }
}

bool get_named_argument_shape(char *args, char *arg_name, list_t **result,
                              list_t *default_value) {
    char shape_str[LEVEL_FILE_ARG_LENGTH];
    char shape_type[LEVEL_FILE_ARG_LENGTH];
    char shape_args[LEVEL_FILE_ARG_LENGTH];
    if (get_named_argument(args, arg_name, shape_str, "")) {
        assert(sscanf(shape_str, "{%[^,],%s}", shape_type, shape_args) == 2);
        if (!strcmp(shape_type, "rect")) {
            char anchor_str[ANCHOR_STR_LENGTH];
            vector_t pos = {.x = 0, .y = 0};
            double width, height;
            assert(sscanf(shape_args, "%[^,],%lf,%lf,%lf,%lf", anchor_str,
                          &pos.x, &pos.y, &width, &height) == 5);
            *result = initialize_rectangle_anchored(
                parse_anchor_option(anchor_str), pos, width, height);
        } else if (!strcmp(shape_type, "star")) {
            char anchor_str[ANCHOR_STR_LENGTH];
            vector_t pos = {.x = 0, .y = 0};
            size_t num_arms;
            double circumradius, inradius;
            assert(sscanf(shape_args, "%[^,],%lf,%lf,%zu,%lf,%lf", anchor_str,
                          &pos.x, &pos.y, &num_arms, &circumradius,
                          &inradius) == 6);
            *result =
                initialize_star_anchored(parse_anchor_option(anchor_str), pos,
                                         num_arms, circumradius, inradius);
        } else {
            // TODO: other shapes
            assert(false);
        }
        return true;
    } else {
        *result = default_value;
        return false;
    }
}

void load_player_paparazzi(state_t *state) {
    assert(state->player);
    list_t *paparazzi_shape = initialize_rectangle_centered(
        body_get_centroid(state->player), PAPARAZZI_WIDTH, PAPARAZZI_HEIGHT);
    body_t *player_paparazzi =
        body_init_with_info(paparazzi_shape, PAPARAZZI_MASS, PAPARAZZI_COLOR,
                            body_info_init(PLAYER_PAPARAZZI), free);
    create_spring(state->scene, PLAYER_PAPARAZZI_SPRING_CONSTANT,
                  player_paparazzi, state->player);
    create_drag(state->scene, PLAYER_PAPARAZZI_DRAG_CONSTANT, player_paparazzi);
    body_set_color(player_paparazzi, COLOR_TRANSPARENT);
    scene_add_body(state->scene, player_paparazzi);
}

void load_level(state_t *state) {
    assert(state->scene);
    scene_clear(state->scene);
    scene_clear(state->hud_scene);
    scene_clear(state->menu_scene);
    sdl_on_key(game_key_handler);
    sdl_on_mouse(game_mouse_handler);
    sdl_play_music(BACKGROUND_MUSIC_FILEPATH);
    state->level_time_elapsed = 0;
    state->game_status = PLAYING;
    char *level_file_path =
        concatenate_strings(LEVEL_FILE_DIR, LEVELS[state->curr_level]);
    FILE *level_file = fopen(level_file_path, "r");
    assert(level_file);
    free(level_file_path);
    bool player_already_created = false;
    body_t *player = NULL;
    while (true) {
        char line[LEVEL_FILE_LINE_LENGTH];
        // Reached end of file
        if (!fgets(line, LEVEL_FILE_LINE_LENGTH, level_file)) {
            break;
        }
        char *command = strtok(line, " ");
        char *args = strtok(NULL, "");
        if (!strcmp(command, "scene_boundary")) {
            bounding_box_t scene_boundary;
            sscanf(args, "%lf %lf %lf %lf", &scene_boundary.min_x,
                   &scene_boundary.min_y, &scene_boundary.max_x,
                   &scene_boundary.max_y);
            state->scene_boundary = scene_boundary;
        } else if (!strcmp(command, "body")) {
            free_func_t freer = free;
            // Role and shape are required
            char role[LEVEL_FILE_ARG_LENGTH];
            assert(get_named_argument(args, "role", role, 0));
            list_t *shape;
            assert(get_named_argument_shape(args, "shape", &shape, NULL));
            double mass;
            get_named_argument_double(args, "mass", &mass, INFINITY);
            rgba_color_t color;
            get_named_argument_color(args, "color", &color, COLOR_BLACK);
            char texture_filename[LEVEL_FILE_ARG_LENGTH];
            get_named_argument(args, "texture", texture_filename, "");
            render_option_t texture_render_option;
            get_named_argument_size_t(args, "texture_render_option",
                                      (size_t *)&texture_render_option,
                                      STRETCH_TO_FIT);
            body_info_t *info;
            if (!strcmp(role, "player")) {
                // There should only be 1 player
                assert(!player_already_created);
                player_already_created = true;
                freer = (free_func_t)player_info_free;
                size_t health;
                assert(get_named_argument_size_t(args, "health", &health, 0));
                double invincibility_time;
                assert(get_named_argument_double(args, "invincibility_time",
                                                 &invincibility_time, 0));
                size_t tongue_damage;
                assert(get_named_argument_size_t(args, "tongue_damage",
                                                 &tongue_damage, 0));
                info = (body_info_t *)player_info_init(
                    health, invincibility_time, tongue_damage);
            } else if (!strcmp(role, "vent")) {
                info = body_info_init(VENT);
            } else if (!strcmp(role, "wall")) {
                info = body_info_init(WALL);
            } else if (!strcmp(role, "damaging_obstacle")) {
                freer = (free_func_t)damaging_obstacle_info_free;
                char *game_over_message =
                    malloc(sizeof(char) * LEVEL_FILE_LINE_LENGTH);
                get_named_argument_str(args, "game_over_message",
                                       game_over_message, GAME_OVER_MESSAGE);
                size_t disappear_upon_player_collision; // (bool)
                get_named_argument_size_t(args, "disappear",
                                          &disappear_upon_player_collision, 0);
                size_t damage;
                assert(get_named_argument_size_t(args, "damage", &damage, 0));
                list_t *trajectory_shape;
                // Default value NULL means the body doesn't have a trajectory
                // to follow.
                get_named_argument_shape(args, "trajectory_shape",
                                         &trajectory_shape, NULL);
                double speed;
                get_named_argument_double(args, "trajectory_speed", &speed, 0);
                trajectory_info_t *trajectory_info =
                    trajectory_info_init(trajectory_shape, speed);
                info = (body_info_t *)damaging_obstacle_info_init(
                    DAMAGING_OBSTACLE, damage, trajectory_info,
                    (bool)disappear_upon_player_collision, game_over_message);
            } else if (!strcmp(role, "crewmate")) {
                size_t health;
                assert(get_named_argument_size_t(args, "health", &health, 0));
                double invincibility_time;
                assert(get_named_argument_double(args, "invincibility_time",
                                                 &invincibility_time, 0));
                list_t *trajectory_shape;
                get_named_argument_shape(args, "trajectory_shape",
                                         &trajectory_shape, NULL);
                double speed;
                get_named_argument_double(args, "trajectory_speed", &speed, 0);
                trajectory_info_t *trajectory_info =
                    trajectory_info_init(trajectory_shape, speed);
                double reload_time;
                assert(get_named_argument_double(args, "reload_time",
                                                 &reload_time, 0));
                size_t damage_per_bullet;
                assert(get_named_argument_size_t(args, "damage_per_bullet",
                                                 &damage_per_bullet, 0));
                char *game_over_message =
                    malloc(sizeof(char) * LEVEL_FILE_LINE_LENGTH);
                get_named_argument_str(args, "game_over_message",
                                       game_over_message, GAME_OVER_MESSAGE);
                size_t facing_left;
                get_named_argument_size_t(args, "facing_left", &facing_left, 0);
                info = (body_info_t *)crewmate_info_init(
                    health, invincibility_time, trajectory_info, reload_time,
                    damage_per_bullet, game_over_message, (bool)facing_left);
            } else if (!strcmp(role, "decoration")) {
                info = body_info_init(DECORATION);
            } else if (!strcmp(role, "key")) {
                size_t id;
                assert(get_named_argument_size_t(args, "id", &id, 0));
                get_named_argument(args, "texture", texture_filename,
                                   KEY_IMAGES[id]);
                info = (body_info_t *)key_and_door_info_init(KEY, id);
            } else if (!strcmp(role, "door")) {
                size_t id;
                assert(get_named_argument_size_t(args, "id", &id, 0));
                get_named_argument_color(args, "color", &color,
                                         DOOR_COLORS[id]);
                info = (body_info_t *)key_and_door_info_init(DOOR, id);
            } else if (!strcmp(role, "trampoline")) {
                double bounciness;
                assert(get_named_argument_double(args, "bounciness",
                                                 &bounciness, 0));
                info = (body_info_t *)trampoline_info_init(bounciness);
            } else {
                assert(false);
            }
            body_t *body = body_init_with_info(shape, mass, color, info, freer);
            if (strlen(texture_filename) > 0) {
                body_set_img_texture(body, texture_filename,
                                     texture_render_option);
            }
            if (get_role(body) == PLAYER) {
                // Delay adding player until the end. This makes it easier
                // to place the player in the beginning of a level file, but
                // have it be seen on top of the background textures
                player = body;
            } else {
                add_body_with_forces(state, body);
            }
        }
    }
    assert(player);
    add_body_with_forces(state, player);
    state->player = player;
    fclose(level_file);
    load_player_paparazzi(state);
}

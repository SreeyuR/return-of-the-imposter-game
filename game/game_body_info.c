#include "game_body_info.h"
#include "game_constants.h"
#include <assert.h>
#include <stdio.h>

#define INITIAL_LIST_SIZE 2

body_info_t *body_info_init(body_role_t role) {
    body_info_t *result = malloc(sizeof(body_info_t));
    assert(result);
    result->role = role;
    return result;
}

body_health_info_t *body_health_info_init(size_t health,
                                          double total_invincibility_time) {
    body_health_info_t *health_info = malloc(sizeof(body_health_info_t));
    assert(health_info);
    health_info->health = health;
    health_info->total_invincibility_time = total_invincibility_time;
    health_info->invincibility_time_left = 0;
    return health_info;
}

damaging_body_info_t *damaging_body_info_init(body_role_t role, size_t damage) {
    damaging_body_info_t *damaging_body_info =
        malloc(sizeof(damaging_body_info_t));
    damaging_body_info->role = role;
    damaging_body_info->damage = damage;
    return damaging_body_info;
}

player_info_t *player_info_init(size_t health, double invincibility_time,
                                size_t tongue_damage) {
    player_info_t *player_info = malloc(sizeof(player_info_t));
    assert(player_info);
    player_info->role = PLAYER;
    player_info->health_info =
        body_health_info_init(health, invincibility_time);
    player_info->tongue_damage = tongue_damage;
    player_info->tongue_timer = 0;
    player_info->tongue_status = READY;
    player_info->key_ids_collected = list_init(INITIAL_LIST_SIZE, free);
    player_info->player_touching_ground = false;
    player_info->facing_left = false;
    return player_info;
}

void player_info_free(player_info_t *player_info) {
    list_free(player_info->key_ids_collected);
    free(player_info->health_info);
    free(player_info);
}

key_and_door_info_t *key_and_door_info_init(body_role_t role, size_t id) {
    key_and_door_info_t *key_and_door_info =
        malloc(sizeof(key_and_door_info_t));
    assert(role & (KEY | DOOR));
    assert(key_and_door_info);
    key_and_door_info->role = role;
    key_and_door_info->id = id;
    return key_and_door_info;
}

trajectory_info_t *trajectory_info_init(list_t *trajectory_shape,
                                        double speed) {
    if (!trajectory_shape) { // trajectory shape is NULL
        return NULL;
    }
    trajectory_info_t *trajectory_info = malloc(sizeof(trajectory_info_t));
    trajectory_info->trajectory_shape = trajectory_shape;
    trajectory_info->speed = speed;
    trajectory_info->curr_point_index = 0;
    return trajectory_info;
}

void trajectory_info_free(trajectory_info_t *trajectory_info) {
    list_free(trajectory_info->trajectory_shape);
    free(trajectory_info);
}

crewmate_info_t *crewmate_info_init(size_t health, double invincibility_time,
                                    trajectory_info_t *trajectory_info,
                                    double reload_time,
                                    size_t damage_per_bullet,
                                    char *game_over_message, bool facing_left) {
    crewmate_info_t *crewmate_info = malloc(sizeof(crewmate_info_t));
    assert(crewmate_info);
    crewmate_info->role = CREWMATE;
    crewmate_info->health_info =
        body_health_info_init(health, invincibility_time);
    crewmate_info->trajectory_info = trajectory_info;
    crewmate_info->reload_time = reload_time;
    crewmate_info->reloading_timer = 0;
    crewmate_info->damage_per_bullet = damage_per_bullet;
    crewmate_info->game_over_message = game_over_message;
    crewmate_info->facing_left = facing_left;
    return crewmate_info;
}

void crewmate_info_free(crewmate_info_t *crewmate_info) {
    free(crewmate_info->game_over_message);
    if (crewmate_info->trajectory_info) {
        trajectory_info_free(crewmate_info->trajectory_info);
    }
    free(crewmate_info->health_info);
    free(crewmate_info);
}

damaging_obstacle_info_t *damaging_obstacle_info_init(
    body_role_t role, size_t damage, trajectory_info_t *trajectory_info,
    bool disappear_upon_player_collision, char *game_over_message) {
    damaging_obstacle_info_t *damaging_obstacle_info =
        malloc(sizeof(damaging_obstacle_info_t));
    damaging_obstacle_info->role = role;
    damaging_obstacle_info->damage = damage;
    damaging_obstacle_info->trajectory_info = trajectory_info;
    damaging_obstacle_info->remove_upon_collision =
        disappear_upon_player_collision;
    damaging_obstacle_info->game_over_message = game_over_message;
    return damaging_obstacle_info;
}

void damaging_obstacle_info_free(
    damaging_obstacle_info_t *damaging_obstacle_info) {
    free(damaging_obstacle_info->game_over_message);
    if (damaging_obstacle_info->trajectory_info) {
        trajectory_info_free(damaging_obstacle_info->trajectory_info);
    }
    free(damaging_obstacle_info);
}

trampoline_info_t *trampoline_info_init(double bounciness) {
    trampoline_info_t *trampoline_info = malloc(sizeof(trampoline_info_t));
    trampoline_info->role = TRAMPOLINE;
    trampoline_info->bounciness = bounciness;
    return trampoline_info;
}

body_role_t get_role(body_t *body) {
    body_role_t body_role = ((body_info_t *)body_get_info(body))->role;
    return body_role;
}

trajectory_info_t *get_trajectory(body_t *body) {
    assert(get_role(body) & (CREWMATE | DAMAGING_OBSTACLE));
    if (get_role(body) == CREWMATE) {
        crewmate_info_t *crewmate_info = (crewmate_info_t *)body_get_info(body);
        return crewmate_info->trajectory_info;
    } else if (get_role(body) == DAMAGING_OBSTACLE) {
        damaging_obstacle_info_t *damaging_obstacle_info =
            (damaging_obstacle_info_t *)body_get_info(body);
        return damaging_obstacle_info->trajectory_info;
    }
    return NULL;
}

vector_t get_curr_trajectory_point(body_t *body) {
    trajectory_info_t *trajectory_info = get_trajectory(body);
    return *(vector_t *)list_get(trajectory_info->trajectory_shape,
                                 trajectory_info->curr_point_index);
}

vector_t get_next_trajectory_point(body_t *body) {
    trajectory_info_t *trajectory_info = get_trajectory(body);
    return *(vector_t *)list_get(
        trajectory_info->trajectory_shape,
        (trajectory_info->curr_point_index + 1) %
            list_size(trajectory_info->trajectory_shape));
}

body_health_info_t *get_health_info(body_t *body) {
    if (get_role(body) == CREWMATE) {
        crewmate_info_t *crewmate_info = body_get_info(body);
        return crewmate_info->health_info;
    } else if (get_role(body) == PLAYER) {
        player_info_t *player_info = body_get_info(body);
        return player_info->health_info;
    } else {
        assert(false);
    }
}

damaging_body_info_t *get_damaging_body_info(body_t *body) {
    assert(get_role(body) & (TONGUE_TIP | DAMAGING_OBSTACLE | BULLET));
    return (damaging_body_info_t *)body_get_info(body);
}

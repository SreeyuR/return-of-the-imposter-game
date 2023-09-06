#ifndef __GAME_BODY_INFO_H__
#define __GAME_BODY_INFO_H__

#include "list.h"
#include "scene.h"
#include <stdbool.h>
#include <stdlib.h>

typedef enum body_role {
    // Individual roles
    PLAYER = 1,
    WALL = 1 << 1,
    VENT = 1 << 2,
    CREWMATE = 1 << 3,
    DAMAGING_OBSTACLE = 1 << 4,
    BULLET = 1 << 5,
    TONGUE = 1 << 6,
    TONGUE_TIP = 1 << 7,
    DECORATION = 1 << 8,
    KEY = 1 << 9,
    DOOR = 1 << 10,
    PLAYER_PAPARAZZI = 1 << 11,
    TRAMPOLINE = 1 << 12,
    // Role combinations
    ANY = (1 << 13) - 1,
    SOLID = ANY & ~(DECORATION | PLAYER_PAPARAZZI | KEY),
} body_role_t;

typedef enum tongue_status {
    READY,
    CHARGING,
    DEPLOYED,
    ATTACHED
} tongue_status_t;

typedef struct body_info {
    body_role_t role;
} body_info_t;

body_info_t *body_info_init(body_role_t role);

typedef struct body_health_info {
    int32_t health;
    double total_invincibility_time;
    double invincibility_time_left;
} body_health_info_t;

typedef struct damaging_body_info {
    body_role_t role;
    size_t damage; // size_t because health is discrete
} damaging_body_info_t;

damaging_body_info_t *damaging_body_info_init(body_role_t role, size_t damage);

typedef struct player_info {
    body_role_t role;
    body_health_info_t *health_info;
    size_t tongue_damage;
    double tongue_timer;
    tongue_status_t tongue_status;
    list_t *key_ids_collected;
    bool player_touching_ground;
    bool facing_left; // if false: moving right
} player_info_t;

player_info_t *player_info_init(size_t health, double invincibility_time,
                                size_t tongue_damage);

void player_info_free(player_info_t *player_info);

typedef struct key_and_door_info {
    body_role_t role;
    size_t id;
} key_and_door_info_t;

key_and_door_info_t *key_and_door_info_init(body_role_t role, size_t id);

typedef struct trajectory_info {
    list_t *trajectory_shape;
    double speed;
    size_t curr_point_index;
} trajectory_info_t;

trajectory_info_t *trajectory_info_init(list_t *trajectory_shape, double speed);

void trajectory_info_free(trajectory_info_t *trajectory_info);

typedef struct crewmate_info {
    body_role_t role;
    body_health_info_t *health_info;
    // assume points in trajectory are in order body follows them in.
    trajectory_info_t *trajectory_info;
    double reload_time;
    double reloading_timer;
    size_t damage_per_bullet;
    char *game_over_message;
    bool facing_left;
} crewmate_info_t;

crewmate_info_t *crewmate_info_init(size_t health, double invincibility_time,
                                    trajectory_info_t *trajectory_info,
                                    double reload_time,
                                    size_t damage_per_bullet,
                                    char *game_over_message, bool facing_left);

void crewmate_info_free(crewmate_info_t *crewmate_info);

typedef struct damaging_obstacle_info {
    body_role_t role;
    size_t damage;
    trajectory_info_t *trajectory_info;
    bool remove_upon_collision;
    char *game_over_message;
} damaging_obstacle_info_t;

damaging_obstacle_info_t *damaging_obstacle_info_init(
    body_role_t role, size_t damage, trajectory_info_t *trajectory_info,
    bool disappear_upon_player_collision, char *game_over_message);

void damaging_obstacle_info_free(
    damaging_obstacle_info_t *damaging_obstacle_info);

typedef struct bullet_info {
    body_role_t role;
    size_t damage;
} bullet_info_t;

typedef struct trampoline_info {
    body_role_t role;
    double bounciness;
} trampoline_info_t;

trampoline_info_t *trampoline_info_init(double bounciness);

body_role_t get_role(body_t *body);

trajectory_info_t *get_trajectory(body_t *body);

vector_t get_curr_trajectory_point(body_t *body);

vector_t get_next_trajectory_point(body_t *body);

body_health_info_t *get_health_info(body_t *body);

damaging_body_info_t *get_damaging_body_info(body_t *body);

#endif // #ifndef __GAME_BODY_INFO_H__

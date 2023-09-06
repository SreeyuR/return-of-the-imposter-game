#ifndef __GAME_H__
#define __GAME_H__

#include "scene.h"
#include <stdbool.h>
#include <stdlib.h>

/**
 * Different statuses the game can be in:
 * MENU - the main menu, the level loading menu, etc are opened
 * PAUSED - the game is paused and pause menu is opened (but the game is still
 *      visible in the background)
 * DEATH - the player died, but the level has not yet reloaded.
 * PLAYING - normal status when playing the game.
 */
typedef enum game_status {
    MENU,
    PAUSED,
    DEATH,
    PLAYING,
} game_status_t;

typedef struct state {
    scene_t *scene;
    scene_t *hud_scene;
    scene_t *menu_scene;
    body_t *player;
    // Table to keep track of which keys are held, bool entry for every char
    bool *held_keys;
    list_t *timers;
    size_t curr_level;
    double level_time_elapsed;
    game_status_t game_status;
    size_t num_deaths_so_far; // this should not be reset when we reset level;
    bounding_box_t scene_boundary;
} state_t;

#endif // #ifndef __GAME_H__

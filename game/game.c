#include "game.h"
#include "game_actions.h"
#include "game_body_info.h"
#include "game_constants.h"
#include "game_gui.h"
#include "game_load_level.h"
#include "game_timers.h"
#include "sdl_wrapper.h"
#include "state.h"
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <time.h>

state_t *emscripten_init() {
    srand(time(0));
    sdl_init();
    state_t *state = malloc(sizeof(state_t));
    assert(state);
    state->scene = scene_init();
    state->hud_scene = scene_init();
    state->menu_scene = scene_init();
    state->player = NULL;
    state->held_keys = malloc(sizeof(bool) * (CHAR_MAX + 1));
    state->timers = list_init(1, free);
    state->curr_level = 0;
    state->level_time_elapsed = 0;
    state->game_status = MENU;
    state->num_deaths_so_far = 0;
    state->scene_boundary = INFINITE_BBOX;
    load_main_menu(state);
    return state;
}

void emscripten_main(state_t *state) {
    sdl_clear();
    if (state->game_status == PLAYING || state->game_status == DEATH) {
        double dt = fmin(time_since_last_tick(), MAX_DT);
        handle_timers(state, dt);
        if (state->game_status == PLAYING) {
            perform_game_actions(state, dt);
            scene_tick(state->scene, dt);
            state->level_time_elapsed += dt;
        }
    }
    // If the game is in progress, load the HUD, set
    // the camera to the scene coordinates and render the game scene. This
    // includes when the game is paused, since it should be visible behind
    // the pause menu.
    if (state->game_status != MENU) {
        // calling this in emsc_main instead of emsc_init because the HUD must
        // be continually updated each frame
        load_hud(state);
        sdl_set_camera_pos(get_camera_for_player_pos(state),
                           state->scene_boundary);
        sdl_render_scene(state->scene);
    }
    // Set the camera to the window coordinates, then render the HUD and the
    // menu scene.
    sdl_set_camera_pos(WINDOW_CENTER, INFINITE_BBOX);
    sdl_render_scene(state->hud_scene);
    sdl_render_scene(state->menu_scene);
    // If the game is in progress, return to the game coordinates. This
    // should not be done when the game is paused, since the user input
    // is expected to come from the menu mouse handler, which is in
    // the window coordinates.
    if (state->game_status == PLAYING || state->game_status == DEATH) {
        sdl_set_camera_pos(get_camera_for_player_pos(state),
                           state->scene_boundary);
    }
}

void emscripten_free(state_t *state) {
    scene_free(state->scene);
    scene_free(state->hud_scene);
    scene_free(state->menu_scene);
    free(state->held_keys);
    list_free(state->timers);
    free(state);
    sdl_free();
}

#ifndef __GAME_ACTIONS_H__
#define __GAME_ACTIONS_H__

#include "sdl_wrapper.h"
#include "vector.h"
#include "game.h"

void game_key_handler(state_t *state, unsigned char key, key_event_type_t type,
                      double held_time);

void game_mouse_handler(state_t *state, mouse_event_type_t type,
                                vector_t mouse_scene_pos,
                                vector_t mouse_prev_scene_pos); 

vector_t get_camera_for_player_pos(state_t *state);

void perform_game_actions(state_t *state, double dt);

#endif // #ifndef __GAME_ACTIONS_H__

#ifndef __GAME_TIMERS_H__
#define __GAME_TIMERS_H__

#include "game.h"

typedef void (*state_func_t)(state_t *state);

void add_timer(state_t *state, double time, state_func_t action);

void handle_timers(state_t *state, double dt);

#endif // #ifndef __GAME_TIMERS_H__

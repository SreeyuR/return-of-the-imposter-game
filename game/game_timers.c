#include "game_timers.h"
#include <stdlib.h>
#include <stdio.h>

/**
 * A timer that performs some action on a state after a given time has passed.
*/
typedef struct game_timer {
    double time_left;
    state_func_t action;
} game_timer_t;

/**
 * Initializes a timer.
*/
game_timer_t *game_timer_init(double time, state_func_t action) {
    game_timer_t *timer = malloc(sizeof(game_timer_t));
    timer->time_left = time;
    timer->action = action;
    return timer;
}

/**
 * Adds a timer to a state's list of timers.
*/
void add_timer(state_t *state, double time, state_func_t action) {
    list_add(state->timers, game_timer_init(time, action));
}

/**
 * Updates all the timers in a state. When some timer's time reaches 0,
 * it is removed and its action is performed.
*/
void handle_timers(state_t *state, double dt) {
    for (size_t i = 0; i < list_size(state->timers); i++) {
        game_timer_t *timer = list_get(state->timers, i);
        timer->time_left -= dt;
        if (timer->time_left <= 0) {
            list_remove(state->timers, i);
            i--;
            timer->action(state);
            free(timer);
        }
    }
}

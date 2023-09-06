#include "game_gui.h"
#include "game_actions.h"
#include "game_body_info.h"
#include "game_constants.h"
#include "game_load_level.h"
#include "polygon.h"
#include "sdl_wrapper.h"
#include <assert.h>
#include <stdlib.h>

// HUD
#define HEART_SIZE 15.0
#define HEART_PADDING_TOP 5.0
#define HEART_PADDING_LEFT 5.0
#define HEART_SPACING 2.0
#define HEART_TEXTURE "resources/sprites/heart.png"

#define PROGRESS_BAR_SHELL_WIDTH 200.0
#define PROGRESS_BAR_SHELL_HEIGHT 15.0
#define PROGRESS_BAR_SHELL_PADDING_TOP 30
#define PROGRESS_BAR_SHELL_PADDING_LEFT 5
#define PROGRESS_BAR_SHELL_COLOR (COLOR_BLACK)
#define PROGRESS_BAR_INTERIOR_PADDING 1.0
#define PROGRESS_BAR_INTERIOR_MAX_WIDTH                                        \
    (PROGRESS_BAR_SHELL_WIDTH - (2 * PROGRESS_BAR_INTERIOR_PADDING))
#define PROGRESS_BAR_INTERIOR_HEIGHT                                           \
    (PROGRESS_BAR_SHELL_HEIGHT - (2 * PROGRESS_BAR_INTERIOR_PADDING))
#define PROGRESS_BAR_INTERIOR_DEPLOYMENT_COLOR (COLOR_ORANGE)
#define PROGRESS_BAR_INTERIOR_CHARGING_COLOR (COLOR_GREEN)

#define KEY_BOX_SIZE 20.0
#define KEY_BOX_SPACING 2.0
#define KEY_BOX_PADDING_TOP 50
#define KEY_BOX_PADDING_LEFT 5

#define LEVEL_TEXT_WIDTH 200.0
#define LEVEL_TEXT_HEIGHT 30.0
#define LEVEL_TEXT_PADDING_TOP 5.0
#define LEVEL_TEXT_PADDING_LEFT                                                \
    (((WINDOW_MIN_X + WINDOW_MAX_X) / 2) - (LEVEL_TEXT_WIDTH / 2))
#define LEVEL_TEXT_FONT_SIZE 30
#define LEVEL_TEXT_COLOR (COLOR_WHITE)
#define LEVEL_TEXT_FONT_PATH "resources/fonts/arial_bold.ttf"

#define LEVEL_TIMER_PADDING_TOP 5
#define LEVEL_TIMER_PADDING_RIGHT 40
#define LEVEL_TIMER_WIDTH 100
#define LEVEL_TIMER_HEIGHT 30
#define LEVEL_TIMER_FONT_PATH "resources/fonts/arial_bold.ttf"
#define LEVEL_TIMER_FONT_SIZE 30
#define LEVEL_TIMER_TEXT_COLOR (COLOR_WHITE)

// Main Menu
#define MAIN_MENU_BUTTON_PADDING_X 250.0
#define MAIN_MENU_BUTTON_PADDING_Y 200.0
#define MAIN_MENU_BUTTON_SPACING 20.0
#define MAIN_MENU_NUM_BUTTONS 2
#define CONTROLS_EXPLANATION_WIDTH 500.0
#define CONTROLS_EXPLANATION_HEIGHT 150.0
#define LEVEL_SELECTION_BACKGROUND_TEXTURE "resources/sprites/space_background_2.jpg"
#define MAIN_MENU_BACKGROUND_TEXTURE "resources/sprites/title_background.png"
#define CONTROLS_EXPLANATION "resources/sprites/controls.png"
#define START_BUTTON_NORMAL "resources/sprites/green_button.png"
#define START_BUTTON_HOVER "resources/sprites/green_button_hover.png"
#define START_BUTTON_CLICKED "resources/sprites/green_button_clicked.png"
#define GO_TO_LEVEL_SELECTION_BUTTON_NORMAL "resources/sprites/green_button.png"
#define GO_TO_LEVEL_SELECTION_BUTTON_HOVER                                     \
    "resources/sprites/green_button_hover.png"
#define GO_TO_LEVEL_SELECTION_BUTTON_CLICKED                                   \
    "resources/sprites/green_button_clicked.png"
#define MAIN_MENU_TEXT_FONT_SIZE 50
#define MAIN_MENU_TEXT_COLOR (COLOR_WHITE)
#define MAIN_MENU_TEXT_FONT_PATH "resources/fonts/arial_bold.ttf"

// Pause Menu
#define PAUSE_MENU_BUTTON_PADDING_X 300.0
#define PAUSE_MENU_BUTTON_PADDING_Y 200.0
#define PAUSE_MENU_BUTTON_SPACING 20.0
#define PAUSE_MENU_NUM_BUTTONS 2
#define PAUSE_MENU_BACKGROUND_TEXTURE                                          \
    "resources/sprites/gray_semitransparent.png"
#define RESUME_BUTTON_NORMAL "resources/sprites/green_button.png"
#define RESUME_BUTTON_HOVER "resources/sprites/green_button_hover.png"
#define RESUME_BUTTON_CLICKED "resources/sprites/green_button_clicked.png"
#define QUIT_BUTTON_NORMAL "resources/sprites/red_button.png"
#define QUIT_BUTTON_HOVER "resources/sprites/red_button_hover.png"
#define QUIT_BUTTON_CLICKED "resources/sprites/red_button_clicked.png"

// Level Selection Menu
#define LEVEL_MENU_BUTTON_WIDTH 50.0
#define LEVEL_MENU_BUTTON_HEIGHT 50.0
#define LEVEL_MENU_BUTTON_SPACING 20.0
#define LEVEL_MENU_BACKGROUND_TEXTURE "resources/sprites/space_background.jpg"
#define LEVEL_BUTTON_NORMAL "resources/sprites/green_button.png"
#define LEVEL_BUTTON_HOVER "resources/sprites/green_button_hover.png"
#define LEVEL_BUTTON_CLICKED "resources/sprites/green_button_clicked.png"

// Victory Screen
#define VICTORY_SCREEN_BACKGROUND_TEXTURE "resources/sprites/victory_background.png"
#define VICTORY_TEXT_WIDTH 500.0
#define VICTORY_TEXT_HEIGHT 150.0
#define VICTORY_TEXT_FONT_SIZE 50
#define VICTORY_TEXT_COLOR (COLOR_WHITE)
#define VICTORY_TEXT_FONT_PATH "resources/fonts/arial_bold.ttf"
#define VICTORY_BUTTON_PADDING_X 300.0
#define VICTORY_BUTTON_PADDING_Y 200.0
#define VICTORY_BUTTON_SPACING 20.0
#define VICTORY_NUM_BUTTONS 1
#define VICTORY_BUTTON_NORMAL "resources/sprites/green_button.png"
#define VICTORY_BUTTON_HOVER "resources/sprites/green_button_hover.png"
#define VICTORY_BUTTON_CLICKED "resources/sprites/green_button_clicked.png"

// Misc
#define LEVEL_STR_MAX_DIGITS 5

typedef enum button_action {
    GO_TO_MAIN_MENU,
    GO_TO_LEVEL_SELECTION,
    LOAD_LEVEL,
    RESUME_GAME
} button_action_t;

typedef struct button_info {
    button_action_t action;
    char *normal_texture;
    char *hover_texture;
    char *clicked_texture;
} button_info_t;

typedef struct load_level_button_info {
    button_action_t action;
    char *normal_texture;
    char *hover_texture;
    char *clicked_texture;
    size_t level;
} load_level_button_info_t;

void display_player_health(state_t *state, body_t *player) {
    body_health_info_t *body_health_info = get_health_info(player);
    for (size_t i = 0; i < body_health_info->health; i++) {
        // anchor to top left corner
        anchor_option_t anchor_option = {.x_anchor = ANCHOR_MIN,
                                         .y_anchor = ANCHOR_MAX};
        list_t *heart_shape = initialize_rectangle_anchored(
            anchor_option,
            (vector_t){.x = WINDOW_MIN_X + HEART_PADDING_LEFT +
                            (HEART_SIZE + HEART_SPACING) * i,
                       .y = WINDOW_MAX_Y - HEART_PADDING_TOP},
            HEART_SIZE, HEART_SIZE);
        body_t *heart = body_init(heart_shape, 0, COLOR_RED);
        body_set_img_texture(heart, HEART_TEXTURE, STRETCH_TO_FIT);
        scene_add_body(state->hud_scene, heart);
    }
}

void display_keys_collected(state_t *state, body_t *player) {
    player_info_t *player_info = body_get_info(player);
    list_t *keys_obtained = player_info->key_ids_collected;
    for (size_t i = 0; i < list_size(keys_obtained); i++) {
        anchor_option_t anchor_option = {.x_anchor = ANCHOR_MIN,
                                         .y_anchor = ANCHOR_MAX};
        list_t *key_box_shape = initialize_rectangle_anchored(
            anchor_option,
            (vector_t){.x = WINDOW_MIN_X + KEY_BOX_PADDING_LEFT +
                            (KEY_BOX_SIZE + KEY_BOX_SPACING) * i,
                       .y = WINDOW_MAX_Y - KEY_BOX_PADDING_TOP},
            KEY_BOX_SIZE, KEY_BOX_SIZE);
        body_t *key = body_init(key_box_shape, 0, COLOR_BLACK);
        size_t id = *(size_t *)list_get(keys_obtained, i);
        body_set_img_texture(key, KEY_IMAGES[id], STRETCH_TO_FIT);
        scene_add_body(state->hud_scene, key);
    }
}

void display_tongue_timer_progress_bar(state_t *state, body_t *player) {
    player_info_t *player_info = body_get_info(player);
    tongue_status_t status = player_info->tongue_status;
    double curr_charge_time = player_info->tongue_timer;
    anchor_option_t progress_bar_anchor_option = {.x_anchor = ANCHOR_MIN,
                                                  .y_anchor = ANCHOR_MAX};

    // progress bar shell
    list_t *progress_bar_shell_shape = initialize_rectangle_anchored(
        progress_bar_anchor_option,
        (vector_t){.x = WINDOW_MIN_X + PROGRESS_BAR_SHELL_PADDING_LEFT,
                   .y = WINDOW_MAX_Y - PROGRESS_BAR_SHELL_PADDING_TOP},
        PROGRESS_BAR_SHELL_WIDTH, PROGRESS_BAR_SHELL_HEIGHT);
    body_t *progress_bar_outline =
        body_init(progress_bar_shell_shape, 0, PROGRESS_BAR_SHELL_COLOR);
    scene_add_body(state->hud_scene, progress_bar_outline);

    // now fill the "shell" with the actual contents of the bar
    size_t progress_remaining_width;
    if ((status == DEPLOYED) || (status == ATTACHED)) {
        progress_remaining_width = (curr_charge_time / TONGUE_DEPLOYMENT_TIME) *
                                   PROGRESS_BAR_INTERIOR_MAX_WIDTH;
        list_t *progress_bar_timer_shape = initialize_rectangle_anchored(
            progress_bar_anchor_option,
            (vector_t){.x = WINDOW_MIN_X + PROGRESS_BAR_SHELL_PADDING_LEFT +
                            PROGRESS_BAR_INTERIOR_PADDING,
                       .y = WINDOW_MAX_Y - PROGRESS_BAR_SHELL_PADDING_TOP -
                            PROGRESS_BAR_INTERIOR_PADDING},
            progress_remaining_width, PROGRESS_BAR_INTERIOR_HEIGHT);
        body_t *progress_bar_timer =
            body_init(progress_bar_timer_shape, 0,
                      PROGRESS_BAR_INTERIOR_DEPLOYMENT_COLOR);
        scene_add_body(state->hud_scene, progress_bar_timer);
    } else {
        if (status == CHARGING) {
            progress_remaining_width =
                ((1 - curr_charge_time) / TONGUE_CHARGE_TIME) *
                PROGRESS_BAR_INTERIOR_MAX_WIDTH;
        } else {
            progress_remaining_width = PROGRESS_BAR_INTERIOR_MAX_WIDTH;
        }
        list_t *progress_bar_timer_shape = initialize_rectangle_anchored(
            progress_bar_anchor_option,
            (vector_t){.x = WINDOW_MIN_X + PROGRESS_BAR_SHELL_PADDING_LEFT +
                            PROGRESS_BAR_INTERIOR_PADDING,
                       .y = WINDOW_MAX_Y - PROGRESS_BAR_SHELL_PADDING_TOP -
                            PROGRESS_BAR_INTERIOR_PADDING},
            progress_remaining_width, PROGRESS_BAR_INTERIOR_HEIGHT);
        body_t *progress_bar_timer = body_init(
            progress_bar_timer_shape, 0, PROGRESS_BAR_INTERIOR_CHARGING_COLOR);
        scene_add_body(state->hud_scene, progress_bar_timer);
    }
}

void display_current_game_level(state_t *state) {
    // anchor to top left corner
    anchor_option_t level_anchor_option = {.x_anchor = ANCHOR_MIN,
                                           .y_anchor = ANCHOR_MAX};
    list_t *background_shape = initialize_rectangle_anchored(
        level_anchor_option,
        (vector_t){.x = WINDOW_MIN_X + LEVEL_TEXT_PADDING_LEFT,
                   .y = WINDOW_MAX_Y - LEVEL_TEXT_PADDING_TOP},
        LEVEL_TEXT_WIDTH, LEVEL_TEXT_HEIGHT);
    body_t *background = body_init(background_shape, 0, COLOR_BLACK);
    scene_add_body(state->hud_scene, background);

    // concatenate the string "LEVEL" with the actual level of the game
    char concatenated_str[6 + LEVEL_STR_MAX_DIGITS];
    sprintf(concatenated_str, "LEVEL %zu", state->curr_level + 1);

    body_set_text_texture(background, concatenated_str, LEVEL_TEXT_FONT_PATH,
                          LEVEL_TEXT_FONT_SIZE, LEVEL_TEXT_COLOR,
                          PRESERVE_ASPECT_RATIO_AND_EXPAND);
}

void display_curr_level_time_elapsed(state_t *state) {
    double total_seconds_elapsed = state->level_time_elapsed;
    int minutes = total_seconds_elapsed / 60;
    int seconds = total_seconds_elapsed - (minutes * 60);
    char timer_text[100];
    sprintf(timer_text, "%02d:%02d", minutes, seconds);

    // anchor to top right corner
    anchor_option_t anchor_option = {.x_anchor = ANCHOR_MAX,
                                     .y_anchor = ANCHOR_MAX};
    list_t *timer_rect = initialize_rectangle_anchored(
        anchor_option,
        (vector_t){.x = WINDOW_MAX_X - LEVEL_TIMER_PADDING_RIGHT,
                   .y = WINDOW_MAX_Y - LEVEL_TIMER_PADDING_TOP},
        LEVEL_TIMER_WIDTH, LEVEL_TIMER_HEIGHT);
    body_t *level_timer = body_init(timer_rect, 0, COLOR_BLACK);
    body_set_text_texture(level_timer, timer_text, LEVEL_TIMER_FONT_PATH,
                          LEVEL_TIMER_FONT_SIZE, LEVEL_TIMER_TEXT_COLOR,
                          PRESERVE_ASPECT_RATIO_AND_EXPAND);
    scene_add_body(state->hud_scene, level_timer);
}

void load_hud(state_t *state) {
    assert(state->hud_scene);
    scene_clear(state->hud_scene);
    body_t *player = state->player;
    assert(player);

    display_player_health(state, player);
    display_keys_collected(state, player);
    display_current_game_level(state);
    display_tongue_timer_progress_bar(state, player);
    display_curr_level_time_elapsed(state);
}

button_info_t *button_info_init(button_action_t action, char *normal_texture,
                                char *hover_texture, char *clicked_texture) {
    button_info_t *result = malloc(sizeof(button_info_t));
    assert(result);
    result->action = action;
    result->normal_texture = normal_texture;
    result->hover_texture = hover_texture;
    result->clicked_texture = clicked_texture;
    return result;
}

load_level_button_info_t *load_level_button_info_init(char *normal_texture,
                                                      char *hover_texture,
                                                      char *clicked_texture,
                                                      size_t level) {
    load_level_button_info_t *result = malloc(sizeof(load_level_button_info_t));
    assert(result);
    result->action = LOAD_LEVEL;
    result->normal_texture = normal_texture;
    result->hover_texture = hover_texture;
    result->clicked_texture = clicked_texture;
    result->level = level;
    return result;
}

void menu_mouse_handler(state_t *state, mouse_event_type_t type,
                        vector_t mouse_scene_pos,
                        vector_t mouse_prev_scene_pos) {
    for (size_t i = 0; i < scene_bodies(state->menu_scene); i++) {
        body_t *body = scene_get_body(state->menu_scene, i);
        button_info_t *button_info = body_get_info(body);
        if (button_info) {
            if (bounding_box_contains_point(body_get_bounding_box(body),
                                            mouse_scene_pos)) {
                if (type == MOUSE_PRESSED) {
                    body_set_img_texture(body, button_info->clicked_texture,
                                         STRETCH_TO_FIT);
                } else if (type == MOUSE_RELEASED) {
                    switch (button_info->action) {
                        case LOAD_LEVEL:
                            state->curr_level =
                                ((load_level_button_info_t *)button_info)
                                    ->level;
                            load_level(state);
                            break;
                        case GO_TO_MAIN_MENU:
                            load_main_menu(state);
                            break;
                        case RESUME_GAME:
                            sdl_resume_music();
                            state->game_status = PLAYING;
                            sdl_on_key(game_key_handler);
                            sdl_on_mouse(game_mouse_handler);
                            scene_clear(state->menu_scene);
                            break;
                        case GO_TO_LEVEL_SELECTION:
                            load_level_selection_menu(state);
                            break;
                    }
                } else if (type == MOUSE_MOVED) {
                    body_set_img_texture(body, button_info->hover_texture,
                                         STRETCH_TO_FIT);
                }
            } else if (bounding_box_contains_point(body_get_bounding_box(body),
                                                   mouse_prev_scene_pos)) {
                body_set_img_texture(body, button_info->normal_texture,
                                     STRETCH_TO_FIT);
            }
        }
    }
}

void menu_key_handler(state_t *state, unsigned char key, key_event_type_t type,
                      double held_time) {
    bool previously_held = state->held_keys[key];
    if (type == KEY_PRESSED) {
        state->held_keys[key] = true;
        // For newly pressed keys that should only trigger once:
        if (!previously_held) {
            switch (key) {
                case 'p':
                    if (state->game_status == PAUSED) {
                        sdl_resume_music();
                        state->game_status = PLAYING;
                        sdl_on_key(game_key_handler);
                        sdl_on_mouse(game_mouse_handler);
                        scene_clear(state->menu_scene);
                    }
                    break;
            }
        }
    } else if (type == KEY_RELEASED) {
        state->held_keys[key] = false;
    }
}

list_t *create_button_shape(double padding_x, double padding_y, double spacing,
                            size_t num_buttons, size_t index) {
    anchor_option_t anchor_option = {.x_anchor = ANCHOR_MIN,
                                     .y_anchor = ANCHOR_MAX};
    double button_width = WINDOW_WIDTH - 2 * padding_x;
    double button_height =
        (WINDOW_HEIGHT - 2 * padding_y - spacing) / (num_buttons);
    return initialize_rectangle_anchored(
        anchor_option,
        (vector_t){.x = WINDOW_MIN_X + padding_x,
                   .y = WINDOW_MAX_Y - padding_y -
                        (button_height + spacing) * index},
        button_width, button_height);
}

void load_main_menu(state_t *state) {
    scene_clear(state->scene);
    scene_clear(state->hud_scene);
    scene_clear(state->menu_scene);
    sdl_on_key(menu_key_handler);
    sdl_on_mouse(menu_mouse_handler);
    state->game_status = MENU;
    body_t *background =
        body_init(initialize_rectangle(WINDOW_MIN_X, WINDOW_MIN_Y, WINDOW_MAX_X,
                                       WINDOW_MAX_Y),
                  0, COLOR_BLACK);
    body_set_img_texture(background, MAIN_MENU_BACKGROUND_TEXTURE,
                         PRESERVE_ASPECT_RATIO_AND_EXPAND);
    scene_add_body(state->menu_scene, background);

    body_t *start_button = body_init_with_info(
        create_button_shape(MAIN_MENU_BUTTON_PADDING_X,
                            MAIN_MENU_BUTTON_PADDING_Y,
                            MAIN_MENU_BUTTON_SPACING, MAIN_MENU_NUM_BUTTONS, 0),
        0, COLOR_BLACK,
        load_level_button_info_init(START_BUTTON_NORMAL, START_BUTTON_HOVER,
                                    START_BUTTON_CLICKED, 0),
        free);
    body_set_img_texture(start_button, START_BUTTON_NORMAL, STRETCH_TO_FIT);
    body_set_text_texture(start_button, "START", MAIN_MENU_TEXT_FONT_PATH,
                          MAIN_MENU_TEXT_FONT_SIZE, MAIN_MENU_TEXT_COLOR,
                          PRESERVE_ASPECT_RATIO_AND_EXPAND);
    scene_add_body(state->menu_scene, start_button);

    body_t *go_to_level_selection_button = body_init_with_info(
        create_button_shape(MAIN_MENU_BUTTON_PADDING_X,
                            MAIN_MENU_BUTTON_PADDING_Y,
                            MAIN_MENU_BUTTON_SPACING, MAIN_MENU_NUM_BUTTONS, 1),
        0, COLOR_BLACK,
        button_info_init(GO_TO_LEVEL_SELECTION,
                         GO_TO_LEVEL_SELECTION_BUTTON_NORMAL,
                         GO_TO_LEVEL_SELECTION_BUTTON_HOVER,
                         GO_TO_LEVEL_SELECTION_BUTTON_CLICKED),
        free);
    body_set_img_texture(go_to_level_selection_button,
                         GO_TO_LEVEL_SELECTION_BUTTON_NORMAL, STRETCH_TO_FIT);
    body_set_text_texture(go_to_level_selection_button, "LEVEL SELECTION",
                          MAIN_MENU_TEXT_FONT_PATH, MAIN_MENU_TEXT_FONT_SIZE,
                          MAIN_MENU_TEXT_COLOR,
                          PRESERVE_ASPECT_RATIO_AND_EXPAND);
    scene_add_body(state->menu_scene, go_to_level_selection_button);

    // body_t *controls_explanation =
    //     body_init(initialize_rectangle_anchored(
    //                   (anchor_option_t){.x_anchor = ANCHOR_CENTER,
    //                                     .y_anchor = ANCHOR_MIN},
    //                   (vector_t){.x = WINDOW_CENTER_X, .y = WINDOW_MIN_Y},
    //                   CONTROLS_EXPLANATION_WIDTH, CONTROLS_EXPLANATION_HEIGHT),
    //               0, COLOR_BLACK);
    // body_set_img_texture(controls_explanation, CONTROLS_EXPLANATION,
    //                      PRESERVE_ASPECT_RATIO_AND_EXPAND);
    // scene_add_body(state->menu_scene, controls_explanation);
}

void load_pause_menu(state_t *state) {
    // Don't clear the game and HUD scenes, so they will be
    // visible in the background
    sdl_pause_music();
    scene_clear(state->menu_scene);
    sdl_on_key(menu_key_handler);
    sdl_on_mouse(menu_mouse_handler);
    state->game_status = PAUSED;
    body_t *background =
        body_init(initialize_rectangle(WINDOW_MIN_X, WINDOW_MIN_Y, WINDOW_MAX_X,
                                       WINDOW_MAX_Y),
                  0, COLOR_BLACK);
    body_set_img_texture(background, PAUSE_MENU_BACKGROUND_TEXTURE,
                         PRESERVE_SCALE_AND_TILE);
    scene_add_body(state->menu_scene, background);
    body_t *resume_button = body_init_with_info(
        create_button_shape(
            PAUSE_MENU_BUTTON_PADDING_X, PAUSE_MENU_BUTTON_PADDING_Y,
            PAUSE_MENU_BUTTON_SPACING, PAUSE_MENU_NUM_BUTTONS, 0),
        0, COLOR_BLACK,
        button_info_init(RESUME_GAME, RESUME_BUTTON_NORMAL, RESUME_BUTTON_HOVER,
                         RESUME_BUTTON_CLICKED),
        free);
    body_set_img_texture(resume_button, RESUME_BUTTON_NORMAL, STRETCH_TO_FIT);
    body_set_text_texture(resume_button, "RESUME GAME",
                          MAIN_MENU_TEXT_FONT_PATH, MAIN_MENU_TEXT_FONT_SIZE,
                          MAIN_MENU_TEXT_COLOR,
                          PRESERVE_ASPECT_RATIO_AND_EXPAND);
    scene_add_body(state->menu_scene, resume_button);
    body_t *quit_to_main_menu_button = body_init_with_info(
        create_button_shape(
            PAUSE_MENU_BUTTON_PADDING_X, PAUSE_MENU_BUTTON_PADDING_Y,
            PAUSE_MENU_BUTTON_SPACING, PAUSE_MENU_NUM_BUTTONS, 1),
        0, COLOR_BLACK,
        button_info_init(GO_TO_MAIN_MENU, QUIT_BUTTON_NORMAL, QUIT_BUTTON_HOVER,
                         QUIT_BUTTON_CLICKED),
        free);
    body_set_img_texture(quit_to_main_menu_button, QUIT_BUTTON_NORMAL,
                         STRETCH_TO_FIT);
    body_set_text_texture(quit_to_main_menu_button, "QUIT TO MAIN MENU",
                          MAIN_MENU_TEXT_FONT_PATH, MAIN_MENU_TEXT_FONT_SIZE,
                          MAIN_MENU_TEXT_COLOR,
                          PRESERVE_ASPECT_RATIO_AND_EXPAND);
    scene_add_body(state->menu_scene, quit_to_main_menu_button);
}

void load_level_selection_menu(state_t *state) {
    scene_clear(state->scene);
    scene_clear(state->hud_scene);
    scene_clear(state->menu_scene);
    sdl_on_key(menu_key_handler);
    sdl_on_mouse(menu_mouse_handler);
    state->game_status = MENU;
    body_t *background =
        body_init(initialize_rectangle(WINDOW_MIN_X, WINDOW_MIN_Y, WINDOW_MAX_X,
                                       WINDOW_MAX_Y),
                  0, COLOR_BLACK);
    body_set_img_texture(background, LEVEL_SELECTION_BACKGROUND_TEXTURE,
                         STRETCH_TO_FIT);
    scene_add_body(state->menu_scene, background);
    const size_t buttons_per_row =
        (WINDOW_WIDTH - LEVEL_MENU_BUTTON_SPACING) /
        (LEVEL_MENU_BUTTON_WIDTH + LEVEL_MENU_BUTTON_SPACING);
    for (size_t level = 0; level < NUM_LEVELS; level++) {
        body_t *button = body_init_with_info(
            initialize_rectangle_anchored(
                (anchor_option_t){.x_anchor = ANCHOR_MIN,
                                  .y_anchor = ANCHOR_MAX},
                (vector_t){
                    .x = WINDOW_MIN_X + LEVEL_MENU_BUTTON_SPACING +
                         (LEVEL_MENU_BUTTON_WIDTH + LEVEL_MENU_BUTTON_SPACING) *
                             (level % buttons_per_row),
                    .y = WINDOW_MAX_Y - LEVEL_MENU_BUTTON_SPACING -
                         (LEVEL_MENU_BUTTON_WIDTH + LEVEL_MENU_BUTTON_SPACING) *
                             (level / buttons_per_row)},
                LEVEL_MENU_BUTTON_WIDTH, LEVEL_MENU_BUTTON_HEIGHT),
            0, COLOR_BLACK,
            load_level_button_info_init(LEVEL_BUTTON_NORMAL, LEVEL_BUTTON_HOVER,
                                        LEVEL_BUTTON_CLICKED, level),
            free);
        body_set_img_texture(button, LEVEL_BUTTON_NORMAL, STRETCH_TO_FIT);
        char button_text[LEVEL_STR_MAX_DIGITS];
        sprintf(button_text, "%zu", level + 1);
        body_set_text_texture(button, button_text, MAIN_MENU_TEXT_FONT_PATH,
                              MAIN_MENU_TEXT_FONT_SIZE, MAIN_MENU_TEXT_COLOR,
                              PRESERVE_ASPECT_RATIO_AND_EXPAND);
        scene_add_body(state->menu_scene, button);
    }
}

void load_victory_screen(state_t *state) {
    sdl_pause_music();
    scene_clear(state->scene);
    scene_clear(state->hud_scene);
    scene_clear(state->menu_scene);
    sdl_on_key(menu_key_handler);
    sdl_on_mouse(menu_mouse_handler);
    state->game_status = MENU;
    body_t *background =
        body_init(initialize_rectangle(WINDOW_MIN_X, WINDOW_MIN_Y, WINDOW_MAX_X,
                                       WINDOW_MAX_Y),
                  0, COLOR_BLACK);
    body_set_img_texture(background, VICTORY_SCREEN_BACKGROUND_TEXTURE,
                         PRESERVE_SCALE_AND_TILE);
    scene_add_body(state->menu_scene, background);
    body_t *victory_button = body_init_with_info(
        create_button_shape(
            VICTORY_BUTTON_PADDING_X, VICTORY_BUTTON_PADDING_Y,
            VICTORY_BUTTON_SPACING, VICTORY_NUM_BUTTONS, 1),
        0, COLOR_BLACK,
        button_info_init(GO_TO_MAIN_MENU, VICTORY_BUTTON_NORMAL, VICTORY_BUTTON_HOVER,
                         VICTORY_BUTTON_CLICKED),
        free);
    body_set_img_texture(victory_button, VICTORY_BUTTON_NORMAL, STRETCH_TO_FIT);
    body_set_text_texture(victory_button, "PLAY AGAIN", VICTORY_TEXT_FONT_PATH,
                          VICTORY_TEXT_FONT_SIZE, VICTORY_TEXT_COLOR,
                          PRESERVE_ASPECT_RATIO_AND_EXPAND);
    scene_add_body(state->menu_scene, victory_button);
}

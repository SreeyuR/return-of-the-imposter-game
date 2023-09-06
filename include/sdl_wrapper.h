#ifndef __SDL_WRAPPER_H__
#define __SDL_WRAPPER_H__

#include "color.h"
#include "list.h"
#include "scene.h"
#include "state.h"
#include "texture_wrapper.h"
#include "vector.h"
#include <stdbool.h>

// Values passed to a key handler when the given arrow key is pressed
typedef enum {
    LEFT_ARROW = 1,
    UP_ARROW = 2,
    RIGHT_ARROW = 3,
    DOWN_ARROW = 4
} arrow_key_t;

/**
 * The possible types of key events.
 * Enum types in C are much more primitive than in Java; this is equivalent to:
 * typedef unsigned int KeyEventType;
 * #define KEY_PRESSED 0
 * #define KEY_RELEASED 1
 */
typedef enum {
    KEY_PRESSED,
    KEY_RELEASED
} key_event_type_t;

typedef enum {
    MOUSE_PRESSED,
    MOUSE_RELEASED,
    MOUSE_MOVED
} mouse_event_type_t;

/**
 * A keypress handler.
 * When a key is pressed or released, the handler is passed its char value.
 * Most keys are passed as their char value, e.g. 'a', '1', or '\r'.
 * Arrow keys have the special values listed above.
 *
 * @param state the state to handle the keypress
 * @param key a character indicating which key was pressed
 * @param type the type of key event (KEY_PRESSED or KEY_RELEASED)
 * @param held_time if a press event, the time the key has been held in seconds
 */
typedef void (*key_handler_t)(state_t *state, unsigned char key, key_event_type_t type,
                              double held_time);

/**
 * A mouse handler.
 * When a mouse is pressed on released, the handler is passed the x, y scene
 * coordinates of the place where the click occurs.
 *
 * @param state the state to handle the mouse click
 * @param type the type of mouse event
 * @param mouse_scene_pos the coordinates of the mouse click in the scene
 * @param mouse_prev_scene_pos the previous coordinates of the mouse click in
 * the scene (before a mouse movement)
 */
typedef void (*mouse_handler_t)(state_t *state, mouse_event_type_t type,
                                vector_t mouse_scene_pos,
                                vector_t mouse_prev_scene_pos);

/**
 * Set window center in terms of scene coordinate to the camera's position.
 * @param new_camera_pos
 */
void sdl_set_camera_pos(vector_t new_camera_pos, bounding_box_t scene_bbox);

/**
 * Set scaling factor for scene coordinate to window coordinate conversion.
 * @param new_zoom > 1 for zooming in and new_zoom < 1 for zooming out.
 * Default value for zoom is 1 (no zoom in or out).
 */
void sdl_set_zoom(double new_zoom);

/**
 * Initializes the SDL window and renderer.
 * Must be called once before any of the other SDL functions.
 *
 */
void sdl_init();

void sdl_free();

/**
 * Processes all SDL events and returns whether the window has been closed.
 * This function must be called in order to handle keypresses.
 *
 * @return true if the window was closed, false otherwise
 */
bool sdl_is_done(state_t *state);

/**
 * Clears the screen. Should be called before drawing polygons in each frame.
 */
void sdl_clear(void);

/**
 * Displays the rendered frame on the SDL window.
 * Must be called after drawing the polygons in order to show them.
 */
void sdl_show(void);

/**
 * Draws all bodies in a scene.
 * This internally calls sdl_clear(), sdl_draw_polygon(), and sdl_show(),
 * so those functions should not be called directly.
 *
 * @param scene the scene to draw
 */
void sdl_render_scene(scene_t *scene);

/**
 * Plays a sound effect using SDL.
 *
 * @param filepath the filepath to the .wav file to be played.
 * @param halt_music if true, the music that is currently playing will be halted
 *                   before the sound effect is played. Otherwise, the sound
 *                   effect will immediately play.
 */
void sdl_play_sound_effect(const char *filepath, bool halt_music);

/**
 * Plays music using SDL.
 *
 * @param filepath the filepath to the .ogg file to be played.
 */
void sdl_play_music(const char *filepath);

/**
 * Pauses the music that is currently playing.
*/
void sdl_pause_music();

/**
 * Resumes the music that was originally playing.
*/
void sdl_resume_music();

/**
 * Registers a function to be called every time a key is pressed.
 * Overwrites any existing handler.
 *
 * Example:
 * ```
 * void on_key(char key, key_event_type_t type, double held_time) {
 *     if (type == KEY_PRESSED) {
 *         switch (key) {
 *             case 'a':
 *                 printf("A pressed\n");
 *                 break;
 *             case UP_ARROW:
 *                 printf("UP pressed\n");
 *                 break;
 *         }
 *     }
 * }
 * int main(void) {
 *     sdl_on_key(on_key);
 *     while (!sdl_is_done());
 * }
 * ```
 *
 * @param handler the function to call with each key press
 */
void sdl_on_key(key_handler_t handler);

/**
 * Registers a function to be called every time a mouse event occurs.
 */
void sdl_on_mouse(mouse_handler_t handler);

/**
 * Gets the amount of time that has passed since the last time
 * this function was called, in seconds.
 *
 * @return the number of seconds that have elapsed
 */
double time_since_last_tick(void);

#endif // #ifndef __SDL_WRAPPER_H__

#ifndef __GAME_CONSTANTS_H__
#define __GAME_CONSTANTS_H__

#include "color.h"

// If this is defined, allow keypresses used for debugging (force crash and skip
// level). Should NOT be defined in the final game!
#define DEBUG_KEYPRESSES

// Window dimensions
#define WINDOW_MIN_X 0
#define WINDOW_MIN_Y 0
#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 500
#define WINDOW_MAX_X (WINDOW_MIN_X + WINDOW_WIDTH)
#define WINDOW_MAX_Y (WINDOW_MIN_Y + WINDOW_HEIGHT)
#define WINDOW_CENTER_X (WINDOW_MIN_X + WINDOW_WIDTH / 2)
#define WINDOW_CENTER_Y (WINDOW_MIN_Y + WINDOW_HEIGHT / 2)
#define WINDOW_CENTER ((vector_t){.x = WINDOW_CENTER_X, .y = WINDOW_CENTER_Y})

// Physical constants
#define GRAVITY_ACCELERATION 100
#define BULLET_GRAVITY_ACCELERATION 5
#define FRICTION_COEFFICIENT 0.3

// Tick time
#define MAX_DT 0.05

// Player data
#define PLAYER_DRAG_CONSTANT 5
#define PLAYER_JUMP_IMPULSE 800
#define PLAYER_MOVE_FORCE 1500

// Camera
#define PAPARAZZI_WIDTH 5
#define PAPARAZZI_HEIGHT 5
#define PAPARAZZI_MASS 0.1
#define PAPARAZZI_COLOR COLOR_RED
#define PLAYER_PAPARAZZI_SPRING_CONSTANT 0.2
#define PLAYER_PAPARAZZI_DRAG_CONSTANT 0.5
#define PLAYER_PAPARAZZI_MAX_RADIUS 250

// Tongue
#define TONGUE_NUM_PIECES 20
#define TONGUE_WIDTH 3
#define TONGUE_SPRING_CONSTANT 0.5
#define TONGUE_DRAG_CONSTANT 0.01
#define TONGUE_ATTACHED_SPRING_CONSTANT 15
#define TONGUE_PIECE_MASS 0.01
#define TONGUE_INITIAL_SPEED 500
#define TONGUE_CHARGE_TIME 1.0
#define TONGUE_DEPLOYMENT_TIME 2.0

#define INVINCIBILITY_BLINKING_TIME 0.1

// Bullets
#define BULLET_WIDTH 5
#define BULLET_HEIGHT 5
#define BULLET_INIT_HORIZONTAL_OFFSET 0
#define BULLET_INIT_VERTICAL_OFFSET 0
#define BULLET_MASS 0.1
#define BULLET_SPEED 100

#define TRAMPOLINE_SIDE_WALL_THICKNESS_RATIO 6
#define TRAMPOLINE_SIDE_WALL_MASS 0

#define GAME_OVER_MESSAGE                                                      \
    "The susness has reached the spikey peaks of insanity. Better luck being " \
    "sus next time.\n"
#define GAME_OVER_TIME_DELAY 4.0

#define BLUE_KEY "resources/sprites/blue_key.png"
#define ORANGE_KEY "resources/sprites/orange_key.png"
#define YELLOW_KEY "resources/sprites/yellow_key.png"
#define RED_KEY "resources/sprites/red_key.png"
#define PURPLE_KEY "resources/sprites/purple_key.png"

#define BACKGROUND_MUSIC_FILEPATH "resources/audio/background_music.ogg"
#define LEVEL_FAILED_SOUND_FILEPATH "resources/audio/level_failed.wav"
#define TONGUE_SOUND_FILEPATH "resources/audio/tongue.wav"
#define JUMP_SOUND_FILEPATH "resources/audio/jump.wav"
#define OOF_SOUND_FILEPATH "resources/audio/oof.wav"
#define OW_SOUND_FILEPATH "resources/audio/ow.wav"
#define KEY_COLLECTED_SOUND_FILEPATH "resources/audio/key_collected.wav"
#define OPEN_DOOR_SOUND_FILEPATH "resources/audio/open_door.wav"
#define WON_LEVEL_SOUND_FILEPATH "resources/audio/won_game.wav"
#define BULLET_SOUND_FILEPATH "resources/audio/bullet.wav"
#define CREWMATE_DEATH_SOUND_FILEPATH "resources/audio/crewmate_death.wav"

extern const char *KEY_IMAGES[];
extern const rgba_color_t DOOR_COLORS[];


// Colors
#define TONGUE_COLOR (hex_to_rgba(0xdb2ed0))
#define BULLET_COLOR COLOR_RED


#endif // __GAME_CONSTANTS_H__

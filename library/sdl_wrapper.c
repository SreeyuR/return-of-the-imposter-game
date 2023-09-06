#include "sdl_wrapper.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

const char WINDOW_TITLE[] = "CS 3";
const static int WINDOW_WIDTH = 1000;
const static int WINDOW_HEIGHT = 500;
const double MS_PER_S = 1e3;

/**
 * The scene coordinate of the window center.
 */
vector_t camera_pos;

/**
 * The scaling factor for converting the scene coordinates to the window
 * coordinates.
 */
double zoom = 1.0;

/**
 * The SDL window where the scene is rendered.
 */
SDL_Window *window;
/**
 * The renderer used to draw the scene.
 */
SDL_Renderer *renderer;
/**
 * The keypress handler, or NULL if none has been configured.
 */
key_handler_t key_handler = NULL;
/**
 * The mouse handler, or NULL if none has been configured.
 */
mouse_handler_t mouse_handler = NULL;
/**
 * SDL's timestamp when a key was last pressed or released.
 * Used to measure how long a key has been held.
 */
uint32_t key_start_timestamp;
/**
 * The value of clock() when time_since_last_tick() was last called.
 * Initially 0.
 */
clock_t last_clock = 0;

// used to initialize SDL_mixer
int mixer_frequency = 22050;
Uint16 mixer_format = MIX_DEFAULT_FORMAT;
int mixer_channels = 2;
int mixer_chunk_size = 4096;

/**
 * The current sound effect and music being played in the game.
 * Whenever a new music or sound effect is played, the respective global
 * variable is updated, and the previous audio Mix_Music or Mix_Chunk is freed.
 */
Mix_Chunk *current_sound_effect = NULL;
Mix_Music *current_music = NULL;

/**
 * Contains the necessary data for rendering text and images using SDL.
 * @var img_texture - contains the necessary data for rendering an image.
 * @var text_texture - contains the necessary data for rendering text in SDL
 *                     using the TTF (TrueType Font) library
 * @var scene_bbox - this coordinate will be updated every body tick. It
 *                   specifies the area within which the text/image is
 *                   constrained.
 * @var img_render_option - specifies how we want to align the image texture
 *                          (ex, stretch to fit)
 * @var text_render_option - specifies how we want to align the text texture
 */
typedef struct texture_wrapper {
    SDL_Texture *img_texture;
    SDL_Texture *text_texture;
    bounding_box_t scene_bbox;
    render_option_t img_render_option;
    render_option_t text_render_option;
    SDL_RendererFlip flip;
    bool visibility;
} texture_wrapper_t;

texture_wrapper_t *texture_wrapper_init(bounding_box_t scene_bbox) {
    texture_wrapper_t *texture_wrapper = malloc(sizeof(texture_wrapper_t));
    assert(texture_wrapper);
    texture_wrapper->img_texture = NULL;
    texture_wrapper->text_texture = NULL;
    texture_wrapper->scene_bbox = scene_bbox;
    texture_wrapper->img_render_option = PRESERVE_ASPECT_RATIO_AND_EXPAND;
    texture_wrapper->text_render_option = PRESERVE_ASPECT_RATIO_AND_EXPAND;
    texture_wrapper->flip = SDL_FLIP_NONE;
    texture_wrapper->visibility = true;
    return texture_wrapper;
}

void texture_wrapper_set_flip(texture_wrapper_t *texture_wrapper,
                              bool horizontal_flip, bool vertical_flip) {
    texture_wrapper->flip = SDL_FLIP_NONE;
    if (horizontal_flip) {
        texture_wrapper->flip |= SDL_FLIP_HORIZONTAL;
    }
    if (vertical_flip) {
        texture_wrapper->flip |= SDL_FLIP_VERTICAL;
    }
}

void texture_wrapper_set_visibility(texture_wrapper_t *texture_wrapper,
                                    bool visibility) {
    texture_wrapper->visibility = visibility;
}

void texture_wrapper_set_img_texture(texture_wrapper_t *texture_wrapper,
                                     const char *img_file,
                                     render_option_t img_render_option) {
    SDL_Texture *texture = IMG_LoadTexture(renderer, img_file);
    if (!texture) {
        printf("Texture loading error: %s\n", SDL_GetError());
    }
    texture_wrapper->img_texture = texture;
    texture_wrapper->img_render_option = img_render_option;
}

void texture_wrapper_set_text_texture(texture_wrapper_t *texture_wrapper,
                                      char *text, const char *font_path,
                                      size_t font_size, rgba_color_t text_color,
                                      render_option_t text_render_option) {
    assert(text);
    TTF_Font *font = TTF_OpenFont(font_path, font_size);
    assert(font);
    SDL_Color color = {text_color.r * 255, text_color.g * 255,
                       text_color.b * 255};
    SDL_Surface *surface = TTF_RenderText_Solid(font, text, color);
    TTF_CloseFont(font);
    assert(surface);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    texture_wrapper->text_texture = texture;
    texture_wrapper->text_render_option = text_render_option;
}

void texture_translate(texture_wrapper_t *texture_wrapper,
                       vector_t translation) {
    texture_wrapper->scene_bbox =
        bounding_box_translate(texture_wrapper->scene_bbox, translation);
}

void texture_wrapper_free(texture_wrapper_t *texture_wrapper) {
    SDL_DestroyTexture(texture_wrapper->img_texture);
    SDL_DestroyTexture(texture_wrapper->text_texture);
    free(texture_wrapper);
}

void sdl_set_camera_pos(vector_t new_camera_pos, bounding_box_t scene_bbox) {
    camera_pos = new_camera_pos;
    if (camera_pos.x < scene_bbox.min_x + WINDOW_WIDTH / 2.0 / zoom) {
        camera_pos.x = scene_bbox.min_x + WINDOW_WIDTH / 2.0 / zoom;
    }
    if (camera_pos.x > scene_bbox.max_x - WINDOW_WIDTH / 2.0 / zoom) {
        camera_pos.x = scene_bbox.max_x - WINDOW_WIDTH / 2.0 / zoom;
    }
    if (camera_pos.y < scene_bbox.min_y + WINDOW_HEIGHT / 2.0 / zoom) {
        camera_pos.y = scene_bbox.min_y + WINDOW_HEIGHT / 2.0 / zoom;
    }
    if (camera_pos.y > scene_bbox.max_y - WINDOW_HEIGHT / 2.0 / zoom) {
        camera_pos.y = scene_bbox.max_y - WINDOW_HEIGHT / 2.0 / zoom;
    }
}

void sdl_set_zoom(double new_zoom) {
    zoom = new_zoom;
}

/** Computes the center of the window in pixel coordinates */
vector_t get_window_center(void) {
    int *width = malloc(sizeof(*width)), *height = malloc(sizeof(*height));
    assert(width != NULL);
    assert(height != NULL);
    SDL_GetWindowSize(window, width, height);
    vector_t dimensions = {.x = *width, .y = *height};
    free(width);
    free(height);
    return vec_multiply(0.5, dimensions);
}

// Scales scene to fit within the window. Don't want to "smush" it anymore.
// 1. Scaling factor: Zoom in/out
// 3. Offset: player moves L/R, Smooth movement (done within the game)

/** Maps a scene coordinate to a window coordinate */
// scene_pos is where the camera scene_coord
vector_t get_window_position(vector_t scene_pos) {
    // Scale scene coordinates by the scaling factor
    // and map the center of the scene to the center of the window
    vector_t window_pos =
        vec_multiply(zoom, vec_subtract(scene_pos, camera_pos));
    window_pos.y *= -1;
    window_pos = vec_add(get_window_center(), window_pos);
    return window_pos;
}

/** Maps a window coordinate to a scene coordinate (in the game). */
vector_t get_scene_position(vector_t window_pos) {
    vector_t scene_pos =
        vec_multiply(1.0 / zoom, vec_subtract(window_pos, get_window_center()));
    scene_pos.y *= -1;
    scene_pos = vec_add(scene_pos, camera_pos);
    return scene_pos;
}

/**
 * Converts an SDL key code to a char.
 * 7-bit ASCII characters are just returned
 * and arrow keys are given special character codes.
 */
char get_keycode(SDL_Keycode key) {
    switch (key) {
        case SDLK_LEFT:
            return LEFT_ARROW;
        case SDLK_UP:
            return UP_ARROW;
        case SDLK_RIGHT:
            return RIGHT_ARROW;
        case SDLK_DOWN:
            return DOWN_ARROW;
        default:
            // Only process 7-bit ASCII characters
            return key == (SDL_Keycode)(char)key ? key : '\0';
    }
}

void sdl_init() {
    SDL_Init(SDL_INIT_EVERYTHING);
    window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH,
                              WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE);
    assert(window);
    TTF_Init();
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    assert(renderer);

    // Initialize SDL_mixer
    assert(Mix_OpenAudio(mixer_frequency, mixer_format, mixer_channels,
                         mixer_chunk_size) != -1);
}

void sdl_free() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

bool sdl_is_done(state_t *state) {
    SDL_Event *event = malloc(sizeof(*event));
    assert(event != NULL);
    while (SDL_PollEvent(event)) {
        switch (event->type) {
            case SDL_QUIT:
                free(event);
                return true;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                // Skip the keypress if no handler is configured
                // or an unrecognized key was pressed
                if (key_handler == NULL)
                    break;
                char key = get_keycode(event->key.keysym.sym);
                if (key == '\0')
                    break;

                uint32_t timestamp = event->key.timestamp;
                if (!event->key.repeat) {
                    key_start_timestamp = timestamp;
                }
                key_event_type_t key_event_type =
                    event->type == SDL_KEYDOWN ? KEY_PRESSED : KEY_RELEASED;
                double held_time = (timestamp - key_start_timestamp) / MS_PER_S;
                key_handler(state, key, key_event_type, held_time);
                break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
            case SDL_MOUSEMOTION:
                if (mouse_handler == NULL) {
                    break;
                }
                mouse_event_type_t mouse_event_type =
                    event->type == SDL_MOUSEBUTTONDOWN ? MOUSE_PRESSED
                    : event->type == SDL_MOUSEBUTTONUP ? MOUSE_RELEASED
                                                       : MOUSE_MOVED;
                vector_t scene_pos = get_scene_position(
                    (vector_t){.x = event->motion.x, .y = event->motion.y});
                vector_t prev_scene_pos = get_scene_position(
                    (vector_t){.x = event->motion.x - event->motion.xrel,
                               .y = event->motion.y - event->motion.yrel});
                mouse_handler(state, mouse_event_type, scene_pos,
                              prev_scene_pos);
                break;
        }
    }
    free(event);
    return false;
}

void sdl_clear(void) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
}

void sdl_draw_texture(texture_wrapper_t *texture_wrapper, vector_t window_pos,
                      double width_in_scene, double height_in_scene,
                      render_option_t render_option, SDL_Texture *texture) {
    if (!texture_wrapper->visibility) {
        return;
    }
    if (width_in_scene <= 0 || height_in_scene <= 0) {
        return;
    }
    int image_width, image_height;
    SDL_QueryTexture(texture, NULL, NULL, &image_width, &image_height);
    int max_width_in_window = width_in_scene * zoom;
    int max_height_in_window = height_in_scene * zoom;
    switch (render_option) {
        case STRETCH_TO_FIT: {
            SDL_Rect target_rect = {.x = window_pos.x,
                                    .y = window_pos.y,
                                    .w = max_width_in_window,
                                    .h = max_height_in_window};
            SDL_Rect src_rect = {
                .x = 0, .y = 0, .w = image_width, .h = image_height};
            assert(SDL_RenderCopyEx(renderer, texture, &src_rect, &target_rect,
                                    0, NULL, texture_wrapper->flip) != -1);
            break;
        }
        case PRESERVE_ASPECT_RATIO_AND_EXPAND: {
            SDL_Rect target_rect;
            SDL_Rect src_rect = {
                .x = 0, .y = 0, .w = image_width, .h = image_height};
            double aspect_ratio = (double)image_width / image_height;
            double target_aspect_ratio =
                (double)max_width_in_window / max_height_in_window;

            if (aspect_ratio > target_aspect_ratio) {
                target_rect.w = max_width_in_window;
                target_rect.h = target_rect.w / aspect_ratio;
            } else {
                target_rect.h = max_height_in_window;
                target_rect.w = target_rect.h * aspect_ratio;
            }
            // center the texture
            target_rect.x =
                window_pos.x + (max_width_in_window - target_rect.w) / 2;
            target_rect.y =
                window_pos.y + (max_height_in_window - target_rect.h) / 2;
            // if (SDL_RenderCopyEx(renderer, texture, &src_rect, &target_rect,
            // 0, NULL, texture_wrapper->flip) == -1) {
            //     printf("%s\n", SDL_GetError());
            // }
            assert(SDL_RenderCopyEx(renderer, texture, &src_rect, &target_rect,
                                    0, NULL, texture_wrapper->flip) == 0);
            break;
        }
        case PRESERVE_SCALE_AND_TILE: {
            SDL_Rect target_rect;
            for (target_rect.x = window_pos.x;
                 target_rect.x < window_pos.x + max_width_in_window;
                 target_rect.x += image_width) {
                for (target_rect.y = window_pos.y;
                     target_rect.y < window_pos.y + max_height_in_window;
                     target_rect.y += image_height) {
                    target_rect.w = image_width;
                    target_rect.h = image_height;
                    if (target_rect.x + target_rect.w - window_pos.x >
                        max_width_in_window) {
                        target_rect.w =
                            window_pos.x + max_width_in_window - target_rect.x;
                    }
                    if (target_rect.y + target_rect.h - window_pos.y >
                        max_height_in_window) {
                        target_rect.h =
                            window_pos.y + max_height_in_window - target_rect.y;
                    }
                    SDL_Rect src_rect = {
                        .x = 0, .y = 0, .w = target_rect.w, .h = target_rect.h};
                    assert(SDL_RenderCopyEx(renderer, texture, &src_rect,
                                            &target_rect, 0, NULL,
                                            texture_wrapper->flip) == 0);
                }
            }
            break;
        }
        default:
            assert(false);
            break;
    }
}

void sdl_draw_image_texture(texture_wrapper_t *texture_wrapper,
                            vector_t window_pos, double width_in_scene,
                            double height_in_scene,
                            render_option_t render_option) {
    sdl_draw_texture(texture_wrapper, window_pos, width_in_scene,
                     height_in_scene, render_option,
                     texture_wrapper->img_texture);
}

void sdl_draw_text_texture(texture_wrapper_t *texture_wrapper,
                           vector_t window_pos, double width_in_scene,
                           double height_in_scene,
                           render_option_t render_option) {
    sdl_draw_texture(texture_wrapper, window_pos, width_in_scene,
                     height_in_scene, render_option,
                     texture_wrapper->text_texture);
}

void sdl_draw_polygon(list_t *points, rgba_color_t color,
                      texture_wrapper_t *texture_wrapper) {
    // Check parameters
    size_t n = list_size(points);
    assert(n >= 3);
    assert(0 <= color.r && color.r <= 1);
    assert(0 <= color.g && color.g <= 1);
    assert(0 <= color.b && color.b <= 1);
    assert(0 <= color.a && color.a <= 1);

    // Convert each vertex to a point on screen
    int16_t *x_points = malloc(sizeof(*x_points) * n),
            *y_points = malloc(sizeof(*y_points) * n);
    assert(x_points != NULL);
    assert(y_points != NULL);
    for (size_t i = 0; i < n; i++) {
        vector_t *vertex = list_get(points, i);
        vector_t pixel = get_window_position(*vertex);
        x_points[i] = pixel.x;
        y_points[i] = pixel.y;
    }

    // Draw image and text textures
    if (texture_wrapper &&
        (texture_wrapper->img_texture || texture_wrapper->text_texture)) {
        bounding_box_t bbox = texture_wrapper->scene_bbox;
        vector_t scene_pos = {.x = bbox.min_x, .y = bbox.max_y};
        double width_in_scene = bbox.max_x - bbox.min_x;
        double height_in_scene = bbox.max_y - bbox.min_y;
        vector_t window_pos = get_window_position(scene_pos);

        // Draw image texture
        if (texture_wrapper->img_texture) {
            sdl_draw_image_texture(texture_wrapper, window_pos, width_in_scene,
                                   height_in_scene,
                                   texture_wrapper->img_render_option);
        }

        // Draw text texture
        if (texture_wrapper->text_texture) {
            sdl_draw_text_texture(texture_wrapper, window_pos, width_in_scene,
                                  height_in_scene,
                                  texture_wrapper->text_render_option);
        }
    }

    // Draw polygon with the given color, if there is no image texture
    else {
        assert(filledPolygonRGBA(renderer, x_points, y_points, n, color.r * 255,
                                 color.g * 255, color.b * 255,
                                 color.a * 255) == 0);
    }
    free(x_points);
    free(y_points);
}

void sdl_play_sound_effect(const char *filepath, bool halt_music) {
    if (halt_music) {
        Mix_HaltMusic();
    }
    if (current_sound_effect) {
        Mix_FreeChunk(current_sound_effect);
    }
    current_sound_effect = Mix_LoadWAV(filepath);
    assert(current_sound_effect);
    if (!current_sound_effect) {
        printf("Sound effect assert failed.%s\n", SDL_GetError());
    }
    int channel = Mix_PlayChannel(-1, current_sound_effect, 0);
    assert(channel != -1);
}

void sdl_play_music(const char *filepath) {
    if (current_music) {
        Mix_FreeMusic(current_music);
    }
    current_music = Mix_LoadMUS(filepath);
    if (!current_music) {
        printf("Current music assert failed.%s\n", SDL_GetError());
    }
    assert(current_music);
    assert(Mix_PlayMusic(current_music, -1) != -1);
    sdl_resume_music();
}

void sdl_pause_music() {
    Mix_PauseMusic();
}

void sdl_resume_music() {
    Mix_ResumeMusic();
}

void sdl_show(void) {
    // Draw boundary lines
    SDL_Rect *boundary = malloc(sizeof(*boundary));
    boundary->x = 0;
    boundary->y = 0;
    boundary->w = WINDOW_WIDTH;
    boundary->h = WINDOW_HEIGHT;
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, boundary);
    free(boundary);

    SDL_RenderPresent(renderer);
}

void sdl_render_scene(scene_t *scene) {
    size_t body_count = scene_bodies(scene);
    for (size_t i = 0; i < body_count; i++) {
        body_t *body = scene_get_body(scene, i);
        list_t *shape = body_get_shape(body);
        sdl_draw_polygon(shape, body_get_color(body), body_get_texture(body));
        sdl_draw_polygon(shape, body_get_color(body), body_get_texture(body));
        list_free(shape);
    }
    sdl_show();
}

void sdl_on_key(key_handler_t handler) {
    key_handler = handler;
}

void sdl_on_mouse(mouse_handler_t handler) {
    mouse_handler = handler;
}

double time_since_last_tick(void) {
    clock_t now = clock();
    double difference = last_clock
                            ? (double)(now - last_clock) / CLOCKS_PER_SEC
                            : 0.0; // return 0 the first time this is called
    last_clock = now;
    return difference;
}

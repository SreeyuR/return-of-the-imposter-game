#ifndef __BODY_H__
#define __BODY_H__

#include "collision.h"
#include "color.h"
#include "list.h"
#include "texture_wrapper.h"
#include "utils.h"
#include "vector.h"
#include <stdbool.h>

/**
 * A rigid body constrained to the plane.
 * Implemented as a polygon with uniform density.
 * Bodies can accumulate forces and impulses during each tick.
 * Angular physics (i.e. torques) are not currently implemented.
 */
typedef struct body body_t;

/**
 * A function called when we want to determine a property of the body that is
 * used outside of the physics engine.
 */
typedef bool (*body_predicate_t)(body_t *body);

/**
 * Initializes a body without any info.
 * Acts like body_init_with_info() where info and info_freer are NULL.
 */
body_t *body_init(list_t *shape, double mass, rgba_color_t color);

/**
 * Allocates memory for a body with the given parameters.
 * The body is initially at rest.
 * Asserts that the mass is positive and that the required memory is allocated.
 *
 * @param shape a list of vectors describing the initial shape of the body
 * @param mass the mass of the body (if INFINITY, stops the body from moving)
 * @param color the color of the body, used to draw it on the screen
 * @param info additional information to associate with the body,
 *   e.g. its type if the scene has multiple types of bodies
 * @param info_freer if non-NULL, a function call on the info to free it
 * @return a pointer to the newly allocated body
 */
body_t *body_init_with_info(list_t *shape, double mass, rgba_color_t color,
                            void *info, free_func_t info_freer);

/**
 * Releases the memory allocated for a body.
 *
 * @param body a pointer to a body returned from body_init()
 */
void body_free(body_t *body);

/**
 * Gets the current shape of a body.
 * Returns a newly allocated vector list, which must be list_free()d.
 *
 * @param body a pointer to a body returned from body_init()
 * @return the polygon describing the body's current position
 */
list_t *body_get_shape(body_t *body);

/**
 * Gets the current center of mass of a body.
 * While this could be calculated with polygon_centroid(), that becomes too slow
 * when this function is called thousands of times every tick.
 * Instead, the body should store its current centroid.
 *
 * @param body a pointer to a body returned from body_init()
 * @return the body's center of mass
 */
vector_t body_get_centroid(body_t *body);

/**
 * Gets the current velocity of a body.
 *
 * @param body a pointer to a body returned from body_init()
 * @return the body's velocity vector
 */
vector_t body_get_velocity(body_t *body);

/**
 * Gets the mass of a body.
 *
 * @param body a pointer to a body returned from body_init()
 * @return the mass passed to body_init(), which must be greater than 0
 */
double body_get_mass(body_t *body);

/**
 * Gets the display color of a body.
 *
 * @param body a pointer to a body returned from body_init()
 * @return the color passed to body_init(), as an (R, G, B) tuple
 */
rgba_color_t body_get_color(body_t *body);

/**
 * Gets the SDL texture of a body.
 *
 * @param body a pointer to a body returned from body_init()
 * @return the `texture_wrapper_t` struct passed to body_init(), where at least
 * one of img_texture or text_texture is non-NULL
 */
texture_wrapper_t *body_get_texture(body_t *body);

/**
 * Gets the bounding box of a body. In other words, gets the rectangular region
 * that fully encloses the body's shape.
 *
 * @param body a pointer to a body returned from body_init()
 * @return the bounding box of a body
 */
bounding_box_t body_get_bounding_box(body_t *body);

/**
 * Gets the current net force acting on body.
 *
 * @param body a pointer to a body returned from body_init()
 * @return the body's net force vector
 */
vector_t body_get_acceleration(body_t *body);

/**
 * Translates all vertices in a solid body by a given vector.
 *
 * @param body a pointer to solid_body that we want to translate
 * @param translation the vector to add to each vertex's position
 */
void body_translate(body_t *body, vector_t translation);

/**
 * Rotates all vertices in a solid body by a given angle.
 *
 * @param body a pointer to solid_body that we want to rotate
 * @param angle the angle to rotate each vertex by
 */
void body_rotate(body_t *body, double angle);

/**
 * Gets the information associated with a body.
 *
 * @param body a pointer to a body returned from body_init()
 * @return the info passed to body_init()
 */
void *body_get_info(body_t *body);

void body_set_texture_flip(body_t *body, bool horizontal_flip, bool vertical_flip);

// TODO: documentation
void body_set_img_texture(body_t *body, const char *img_file, render_option_t img_render_option);

// TODO: documentation
void body_set_text_texture(body_t *body, char *text, const char *font_path,
                           size_t font_size, rgba_color_t text_color, render_option_t text_render_option);

void body_set_visibility(body_t *body, bool visibility);

/**
 * Translates a body to a new position.
 * The position is specified by the position of the body's center of mass.
 *
 * @param body a pointer to a body returned from body_init()
 * @param x the body's new centroid
 */
void body_set_centroid(body_t *body, vector_t x);

/**
 * Changes a body's velocity (the time-derivative of its position).
 *
 * @param body a pointer to a body returned from body_init()
 * @param v the body's new velocity
 */
void body_set_velocity(body_t *body, vector_t v);

/**
 * Changes a body's mass
 *
 * @param body a pointer to a body returned from body_init()
 * @param mass the body's new mass
 */
void body_set_mass(body_t *body, double mass);

/**
 * Changes a body's color
 *
 * @param body a pointer to a body returned from body_init()
 * @param color the body's new color
 */
void body_set_color(body_t *body, rgba_color_t color);

/**
 * Changes a body's angular velocity.
 *
 * @param body a pointer to a body returned from body_init()
 * @param angular_velocity the body's new angular velocity
 */
void body_set_angular_velocity(body_t *body, double angular_velocity);

/**
 * Changes a body's orientation in the plane.
 * The body is rotated about its center of mass.
 * Note that the angle is *absolute*, not relative to the current orientation.
 *
 * @param body a pointer to a body returned from body_init()
 * @param angle the body's new angle in radians. Positive is counterclockwise.
 */
void body_set_rotation(body_t *body, double angle);

/**
 * Applies a force to a body over the current tick.
 * If multiple forces are applied in the same tick, they should be added.
 * Should not change the body's position or velocity; see body_tick().
 *
 * @param body a pointer to a body returned from body_init()
 * @param force the force vector to apply
 */
void body_add_force(body_t *body, vector_t force);

/**
 * Applies an impulse to a body.
 * An impulse causes an instantaneous change in velocity,
 * which is useful for modeling collisions.
 * If multiple impulses are applied in the same tick, they should be added.
 * Should not change the body's position or velocity; see body_tick().
 *
 * @param body a pointer to a body returned from body_init()
 * @param impulse the impulse vector to apply
 */
void body_add_impulse(body_t *body, vector_t impulse);

/**
 * Updates the body after a given time interval has elapsed.
 * Sets acceleration and velocity according to the forces and impulses
 * applied to the body during the tick.
 * The body should be translated at the *average* of the velocities before
 * and after the tick.
 * Resets the forces and impulses accumulated on the body.
 *
 * @param body the body to tick
 * @param dt the number of seconds elapsed since the last tick
 */
void body_tick(body_t *body, double dt);

/**
 * Copy the contents of a body_t object.
 * @param body a pointer to a solid body
 * @return a copy of the solid body
 */
body_t *body_copy(body_t *body);

/**
 * Returns the shape of the body as if it was ticked.
 * Does not modify the original body.
 * @param body a pointer to a solid body
 * @param dt the time that passed
 * @return a polygon representing the shape of the future body
 */
list_t *future_body_trans_rot(body_t *body, double dt);

/**
 * Returns the shape of the body as if it was ticked, but only the translational
 * component was advanced. Does not modify the original body.
 * @param body a pointer to a solid body
 * @param dt the time that passed
 * @return a polygon representing the shape of the future body
 */
list_t *future_body_translational(body_t *body, double dt);

/**
 * Returns the shape of the body as if it was ticked, but only the rotational
 * component was advanced. Does not modify the original body.
 * @param body a pointer to a solid body
 * @param dt the time that passed
 * @return a polygon representing the shape of the future body
 */
list_t *future_body_rotational(body_t *body, double dt);

/**
 * Detects a collision between two bodies.
 * @param body1 the first body
 * @param body2 the second body
 * @return a collision_info_t struct that tells if the bodies collided and if
 * so, the axis of collision
 */
collision_info_t detect_body_collision(body_t *body1, body_t *body2);

/**
 * Marks a body for removal--future calls to body_is_removed() will return true.
 * Does not free the body.
 * If the body is already marked for removal, does nothing.
 *
 * @param body the body to mark for removal
 */
void body_remove(body_t *body);

/**
 * Returns whether a body has been marked for removal.
 * This function returns false until body_remove() is called on the body,
 * and returns true afterwards.
 *
 * @param body the body to check
 * @return whether body_remove() has been called on the body
 */
bool body_is_removed(body_t *body);

#endif // #ifndef __BODY_H__

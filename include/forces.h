#ifndef __FORCES_H__
#define __FORCES_H__

#include "scene.h"

/**
 * A function called when a collision occurs.
 * @param body1 the first body passed to create_collision()
 * @param body2 the second body passed to create_collision()
 * @param axis a unit vector pointing from body1 towards body2
 *   that defines the direction the two bodies are colliding in
 * @param aux the auxiliary value passed to create_collision()
 */
typedef void (*collision_handler_t)(body_t *body1, body_t *body2, vector_t axis,
                                    void *aux);

/**
 * A function called when an interaction occurs between two bodies. Ex, when one
 * body is within the line of sight of another body.
 *
 * @param body1 the body whose point of view we are looking from
 * @param body2 the body we want to determine the visiblity of
 * @param aux the auxiliary value passed to create_special_interaction()
 */
typedef void (*special_interaction_handler_t)(body_t *body1, body_t *body2,
                                              void *aux);

/**
 * Adds a force creator to a scene that applies gravity between two bodies.
 * The force creator will be called each tick
 * to compute the Newtonian gravitational force between the bodies.
 * See
 * https://en.wikipedia.org/wiki/Newton%27s_law_of_universal_gravitation#Vector_form.
 * The force should not be applied when the bodies are very close,
 * because its magnitude blows up as the distance between the bodies goes to 0.
 *
 * @param scene the scene containing the bodies
 * @param G the gravitational proportionality constant
 * @param body1 the first body
 * @param body2 the second body
 */
void create_newtonian_gravity(scene_t *scene, double G, body_t *body1,
                              body_t *body2);

/**
 * Adds a force creator to a scene that applies gravity to a single body.
 * The force creator will be called each tick to compute the force of
 * gravity for that body
 *
 * @param scene the scene containing the body
 * @param g the acceleration due to gravity
 * @param body the body we apply gravity to
 */
void create_global_gravity(scene_t *scene, double g, body_t *body);

/**
 * Adds a force creator to a scene that acts like a spring between two bodies.
 * The force creator will be called each tick
 * to compute the Hooke's-Law spring force between the bodies.
 * See https://en.wikipedia.org/wiki/Hooke%27s_law.
 *
 * @param scene the scene containing the bodies
 * @param k the Hooke's constant for the spring
 * @param body1 the first body
 * @param body2 the second body
 */
void create_spring(scene_t *scene, double k, body_t *body1, body_t *body2);

/**
 * Adds a force creator to a scene that applies a drag force on a body.
 * The force creator will be called each tick
 * to compute the drag force on the body proportional to its velocity.
 * The force points opposite the body's velocity.
 *
 * @param scene the scene containing the bodies
 * @param gamma the proportionality constant between force and velocity
 *   (higher gamma means more drag)
 * @param body the body to slow down
 */
void create_drag(scene_t *scene, double gamma, body_t *body);

/**
 * Creates a rigid constraint between two bodies, making the child body stay
 * at the same position relative to the parent body as when the force was
 * created. This force will not have any effect on the behavior of the parent
 * body
 *  - forces will not be shared
 * between the two bodies. The child body will just be continuously translated
 * to maintain the same distance to the parent body.
 */
void create_physical_rigid_constraint(scene_t *scene, body_t *body1,
                                      body_t *body2);

/**
 * Adds a force creator to a scene that calls a given collision handler
 * function each time two bodies collide.
 * The handler is passed the bodies, the collision axis, and an auxiliary value.
 *
 * @param scene the scene containing the bodies
 * @param body1 the first body
 * @param body2 the second body
 * @param handler a function to call whenever the bodies collide
 * @param is_post_tick if true, the handler is only applied after the body ticks
 * @param is_contact_collision if true, the handler is applied even if the
 * bodies collided in the last frame
 * @param is_full_collision if true, the handler is only applied if the
 * collision is full - that is, the bodies fully overlap
 * @param aux an auxiliary value to pass to the handler
 * @param freer if non-NULL, a function to call in order to free aux
 */
void create_generic_collision(scene_t *scene, body_t *body1, body_t *body2,
                              collision_handler_t handler, bool is_post_tick,
                              bool is_contact_collision, bool is_full_collision,
                              void *aux, free_func_t freer);

/**
 * Adds a force creator to a scene that calls a given collision handler
 * function each time two bodies collide.
 * This generalizes create_destructive_collision() from last week,
 * allowing different things to happen on a collision.
 * The handler is passed the bodies, the collision axis, and an auxiliary value.
 * It should only be called once while the bodies are still colliding.
 *
 * @param scene the scene containing the bodies
 * @param body1 the first body
 * @param body2 the second body
 * @param handler a function to call whenever the bodies collide
 * @param aux an auxiliary value to pass to the handler
 * @param freer if non-NULL, a function to call in order to free aux
 */
void create_collision(scene_t *scene, body_t *body1, body_t *body2,
                      collision_handler_t handler, void *aux,
                      free_func_t freer);

/**
 * Adds a collision handler in the same way as create_collision(), but
 * it is applied in every frame while the bodies are colliding, not just once.
 *
 * @param scene the scene containing the bodies
 * @param body1 the first body
 * @param body2 the second body
 * @param handler a function to call whenever the bodies collide
 * @param aux an auxiliary value to pass to the handler
 * @param freer if non-NULL, a function to call in order to free aux
 */
void create_contact_collision(scene_t *scene, body_t *body1, body_t *body2,
                              collision_handler_t handler, void *aux,
                              free_func_t freer);

/**
 * Adds a force creator to a scene that destroys two bodies when they collide.
 * The bodies should be destroyed by calling body_remove().
 * This should be represented as an on-collision callback
 * registered with create_collision().
 *
 * @param scene the scene containing the bodies
 * @param body1 the first body
 * @param body2 the second body
 */
void create_destructive_collision(scene_t *scene, body_t *body1, body_t *body2);

/**
 * Adds a force creator to a scene that destroys one of two bodies when the
 * two bodies fully collide.
 *
 * @param scene the scene containing the bodies
 * @param body_to_be_destroyed the body that should be destroyed on collision
 * @param other_body the other body
 */
void create_one_body_full_destructive_collision(scene_t *scene,
                                                body_t *body_to_be_destroyed,
                                                body_t *other_body);
/**
 * Adds a friction force creator to a scene 
 *
 * @param scene the scene containing the bodies
 * @param mu the coefficient of friction
 * @param body1 the first body
 * @param body2 the second body
 */
void create_friction(scene_t *scene, double mu, body_t *body1, body_t *body2);

/**
 * Adds a force creator to a scene that applies impulses
 * to resolve collisions between two bodies in the scene.
 * This should be represented as an on-collision callback
 * registered with create_collision().
 *
 * You may remember from project01 that you should avoid applying impulses
 * multiple times while the bodies are still colliding.
 * You should also have a special case that allows either body1 or body2
 * to have mass INFINITY, as this is useful for simulating walls.
 *
 * @param scene the scene containing the bodies
 * @param elasticity the "coefficient of restitution" of the collision;
 * 0 is a perfectly inelastic collision and 1 is a perfectly elastic collision
 * @param body1 the first body
 * @param body2 the second body
 */
void create_physics_collision(scene_t *scene, double elasticity, body_t *body1,
                              body_t *body2);

// TODO: documentation
void create_instant_resolution_collision(scene_t *scene, body_t *body1,
                                         body_t *body2);

/**
 * Adds a force creator to a scene that calls a given collision handler
 * function each time two bodies interact. For example, one body could be within
 * another body's line of sight. The handler is passed the bodies, the collision
 * axis, and an auxiliary value.
 *
 * @param scene the scene containing the bodies
 * @param body1 the first body
 * @param body2 the second body
 * @param handler a function to call whenever the bodies collide
 * @param is_post_tick if true, the handler is only applied after the body ticks
 * @param aux an auxiliary value to pass to the handler
 * @param freer if non-NULL, a function to call in order to free aux
 */
void create_special_interaction(scene_t *scene, body_t *body1, body_t *body2,
                                special_interaction_handler_t handler,
                                bool is_post_tick, void *aux,
                                free_func_t freer);

#endif // #ifndef __FORCES_H__

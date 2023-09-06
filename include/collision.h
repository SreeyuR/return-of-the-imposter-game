#ifndef __COLLISION_H__
#define __COLLISION_H__

#include "list.h"
#include "vector.h"
#include <stdbool.h>

typedef enum {
    NO_COLLISION,
    PARTIAL_COLLISION,
    FULL_COLLISION
} collision_status_t;

/**
 * Represents the status of a collision between two shapes.
 * The shapes are either not colliding, or they are colliding along some axis.
 */
typedef struct {
    /** Whether the two shapes are colliding */
    collision_status_t collided;
    /**
     * If the shapes are colliding, the axis they are colliding on.
     * This is a unit vector pointing from the first shape towards the second.
     * Normal impulses are applied along this axis.
     * If collided is false, this value is undefined.
     */
    vector_t axis;
    // The amount of overlap between the shapes along the 
    // collision axis.
    double overlap;
} collision_info_t;

/**
 * Computes the status of the collision between two convex polygons.
 * The shapes are given as lists of vertices in counterclockwise order.
 * There is an edge between each pair of consecutive vertices,
 * and one between the first vertex and the last vertex.
 *
 * @param shape1 the first shape
 * @param shape2 the second shape
 * @return whether the shapes are colliding, and if so, the collision axis.
 * The axis should be a unit vector pointing from shape1 towards shape2.
 */
collision_info_t find_collision(list_t *shape1, list_t *shape2);

#endif // #ifndef __COLLISION_H__

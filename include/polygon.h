#ifndef __POLYGON_H__
#define __POLYGON_H__

#include "bounding_box.h"
#include "list.h"
#include "vector.h"

typedef enum anchor_option_1d {
    ANCHOR_MIN,
    ANCHOR_CENTER,
    ANCHOR_MAX
} anchor_option_1d_t;

typedef struct anchor_option {
    anchor_option_1d_t x_anchor;
    anchor_option_1d_t y_anchor;
} anchor_option_t;

bounding_box_t polygon_get_bounding_box(list_t *polygon);

/**
 * Computes the area of a polygon.
 * See https://en.wikipedia.org/wiki/Shoelace_formula#Statement.
 *
 * @param polygon the list of vertices that make up the polygon,
 * listed in a counterclockwise direction. There is an edge between
 * each pair of consecutive vertices, plus one between the first and last.
 * @return the area of the polygon
 */
double polygon_area(list_t *polygon);

/**
 * Computes the center of mass of a polygon.
 * See https://en.wikipedia.org/wiki/Centroid#Of_a_polygon.
 *
 * @param polygon the list of vertices that make up the polygon,
 * listed in a counterclockwise direction. There is an edge between
 * each pair of consecutive vertices, plus one between the first and last.
 * @return the centroid of the polygon
 */
vector_t polygon_centroid(list_t *polygon);

/**
 * Translates all vertices in a polygon by a given vector.
 * Note: mutates the original polygon.
 *
 * @param polygon the list of vertices that make up the polygon
 * @param translation the vector to add to each vertex's position
 */
void polygon_translate(list_t *polygon, vector_t translation);

/**
 * Rotates vertices in a polygon by a given angle about a given point.
 * Note: mutates the original polygon.
 *
 * @param polygon the list of vertices that make up the polygon
 * @param angle the angle to rotate the polygon, in radians.
 * A positive angle means counterclockwise.
 * @param point the point to rotate around
 */
void polygon_rotate(list_t *polygon, double angle, vector_t point);

/**
 * Initializes a star-shaped polygon.
 * @param center the center coordinate of the star
 * @param num_arms the number of arms of a star (number of vertices is
 *                 2 * num_arms)
 * @param circumradius the larger radius of the star
 * @param inradius the smaller radius of the star
 * @return a polygon with the given parameters. A point at a distance of
 *          the circumradius lies on x-axis.
 */
list_t *initialize_star(vector_t center, size_t num_arms, double circumradius,
                        double inradius);

list_t *initialize_star_anchored(anchor_option_t anchor, vector_t pos,
                                 size_t num_arms, double circumradius,
                                 double inradius);

list_t *initialize_regular_polygon(vector_t center, double circumradius,
                                   size_t num_verts);

list_t *initialize_pacman(vector_t center, double mouth_angle,
                          double face_radius, size_t num_segments_pacman_back);

list_t *initialize_rectangle(double min_x, double min_y, double max_x,
                             double max_y);

list_t *initialize_rectangle_centered(vector_t center, double width,
                                      double height);

list_t *initialize_rectangle_anchored(anchor_option_t anchor, vector_t pos,
                                      double width, double height);

/**
 * Initializes a rotated rectangle whose 4 sides are not parallel to the x- and
 * y- axes. The rectangle is defined by two points, pos1 and pos2, and has a
 * specified width.
 * @param pos1 The point upon which one of the sides of the rectangle is
 * anchored to.
 * @param pos2 The other point upon which the opposite side of the rectangle is
 * anchored to.
 * @param width The width of the rectangle.
 * @return A list_t pointer that represents the rotated rectangle shape.
 */
list_t *initialize_rectangle_rotated(vector_t pos1, vector_t pos2,
                                     double width);

list_t *initialize_ellipse(vector_t center, double width, double height,
                           size_t num_verts);

list_t *initialize_ellipse_anchored(anchor_option_t anchor, vector_t pos,
                                    double width, double height,
                                    size_t num_verts);

#endif // #ifndef __POLYGON_H__

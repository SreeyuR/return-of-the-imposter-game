#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>

#define PI 3.1415926535897932384626433832795

/**
 * A function that can be called on pointers to release their resources.
 * Examples: free, body_free
 */
typedef void (*free_func_t)(void *);

/**
 * A function that can be called on pointers to copy them.
 */
typedef void *(*copy_func_t)(void *);

/**
 * Generates a random double between two values.
 * @param min the min value the random double could take on
 * @param max the max value the random double could take on
 */
double random_between(double min, double max);

/**
 * Computes the overlap between two 1-dimensional line segments.
 *
 * @param min1 the mininum coordinate of the first line segment
 * @param max1 the maximum coordinate of the first line segment
 * @param min2 the mininum coordinate of the second line segment
 * @param max2 the maximum coordinate of the second line segment
 *
 * @return the amount of overlap (0 if segments don't overlap)
 */
double segment_overlap(double min1, double max1, double min2, double max2);


char *concatenate_strings(const char *str1, const char *str2);

#endif // #ifndef __UTILS_H__

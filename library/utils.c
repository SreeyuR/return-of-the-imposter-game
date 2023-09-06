#include "utils.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

double random_between(double min, double max) {
    assert(min <= max);
    if (min == max) {
        return min;
    }
    return ((double)rand() / RAND_MAX * (max - min)) + min;
}

double segment_overlap(double min1, double max1, double min2, double max2) {
    return fmax(0, fmin(max1, max2) - fmax(min1, min2));
}

char *concatenate_strings(const char *str1, const char *str2) {
    char *result = malloc(sizeof(char) * (strlen(str1) + strlen(str2) + 1));
    strcpy(result, str1);
    strcat(result, str2);
    return result;
}
